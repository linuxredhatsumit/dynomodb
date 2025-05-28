import argparse
import boto3
import json
import sys
import logging
import difflib
from botocore.exceptions import ClientError
from decimal import Decimal

# Set up logging for pipeline
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger()

# Suppress botocore and boto3 INFO logs to remove credential messages
logging.getLogger('botocore').setLevel(logging.WARNING)
logging.getLogger('boto3').setLevel(logging.WARNING)

# Custom JSON encoder to handle Decimal types
class DecimalEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, Decimal):
            return str(obj)  # Convert Decimal to string for logging
        return super(DecimalEncoder, self).default(obj)

def fetch_item(table_name, partition_key, partition_value):
    dynamodb = boto3.resource('dynamodb', region_name='ap-south-1')
    table = dynamodb.Table(table_name)
    try:
        logger.info(f"Fetching item from table: {table_name} with key: {partition_key}={partition_value}")
        response = table.get_item(Key={partition_key: partition_value})
        item = response.get('Item', {})
        if not item:
            logger.error("No item found for the given key.")
            sys.exit(1)
        logger.info("Fetched item: %s", json.dumps(item, indent=4, cls=DecimalEncoder))
        return item
    except ClientError as e:
        logger.error(f"Error fetching item: {e.response['Error']['Message']}")
        sys.exit(1)

def get_nested_item(item, keys):
    """Recursively fetch a nested item using a list of keys, returning the item and the last valid path."""
    current = item
    last_valid_path = []
    for key in keys:
        if not isinstance(current, dict) or key not in current:
            return None, last_valid_path
        last_valid_path.append(key)
        current = current[key]
    return current, last_valid_path

def suggest_similar_key(invalid_key, available_keys):
    """Suggest a similar key using string similarity."""
    if not available_keys:
        return None
    matches = difflib.get_close_matches(invalid_key, available_keys, n=1, cutoff=0.8)
    return matches[0] if matches else None

def create_nested_path(item, key_parts, value):
    """Recursively create a nested path in the item if it doesn't exist."""
    current = item
    for i, key in enumerate(key_parts[:-1]):
        if key not in current:
            current[key] = {}
        current = current[key]
    current[key_parts[-1]] = value
    return item

def infer_dynamodb_type(value, key_parts):
    """Infer the DynamoDB type for the value, especially for the last key part."""
    # If the last key is 'N', treat the value as a Number
    if key_parts[-1] == 'N':
        try:
            return int(value) if value.isdigit() else float(value)
        except ValueError:
            logger.warning(f"Value '{value}' for key 'N' could not be converted to a number; treating as string.")
            return value
    # Default to string if no specific type is inferred
    return value

def update_item(table_name, partition_key, partition_value, update_attribute):
    dynamodb = boto3.resource('dynamodb', region_name='ap-south-1')
    table = dynamodb.Table(table_name)
    try:
        # Split update_attribute into key and value
        key, value = update_attribute.split('=', 1)
        key = key.strip()
        value = value.strip()
        logger.info(f"Attempting to update attribute: {key}={value}")

        # Fetch item to validate key existence
        item = fetch_item(table_name, partition_key, partition_value)

        # Check if the key contains dot notation
        if '.' in key:
            # Split the key into parts for nested update
            key_parts = key.split('.')
            attr = key_parts[0]  # e.g., globalJourney

            # Validate that the top-level attribute exists
            if attr not in item:
                logger.info(f"Top-level attribute '{attr}' does not exist; creating it.")
                item[attr] = {}
                # Update the item in DynamoDB with the new top-level attribute
                table.update_item(
                    Key={partition_key: partition_value},
                    UpdateExpression="SET #attr = :val",
                    ExpressionAttributeNames={"#attr": attr},
                    ExpressionAttributeValues={":val": item[attr]},
                    ReturnValues="ALL_NEW"
                )
                item = fetch_item(table_name, partition_key, partition_value)

            # Check the nested path
            nested_item, last_valid_path = get_nested_item(item, key_parts[:-1])  # Check up to the second-to-last key
            if nested_item is None or not isinstance(nested_item, dict) or key_parts[-1] not in nested_item:
                # Determine where the path failed
                failed_index = len(last_valid_path)
                failed_key = key_parts[failed_index]
                last_valid_item = get_nested_item(item, key_parts[:failed_index])[0] if failed_index > 0 else item
                available_keys = list(last_valid_item.keys()) if isinstance(last_valid_item, dict) else []
                suggestion = suggest_similar_key(failed_key, available_keys)
                if suggestion:
                    logger.warning(f"Correcting typo: '{failed_key}' to '{suggestion}' in path '{key}'")
                    key_parts[failed_index] = suggestion
                    key = ".".join(key_parts)
                    # Revalidate the path after correction
                    nested_item, _ = get_nested_item(item, key_parts[:-1])
                    if nested_item is None or not isinstance(nested_item, dict):
                        logger.error(f"Corrected path '{key}' still invalid after typo correction. Update aborted.")
                        sys.exit(1)

                # If the path doesn't exist, create it
                if key_parts[-1] not in nested_item:
                    logger.info(f"Nested attribute '{key}' does not exist; creating the path.")
                    value_to_set = infer_dynamodb_type(value, key_parts)
                    updated_item = create_nested_path(item, key_parts, value_to_set)
                    # Update the entire item in DynamoDB
                    response = table.update_item(
                        Key={partition_key: partition_value},
                        UpdateExpression="SET #attr = :val",
                        ExpressionAttributeNames={"#attr": attr},
                        ExpressionAttributeValues={":val": updated_item[attr]},
                        ReturnValues="ALL_NEW"
                    )
                    logger.info("Attribute created and updated successfully!")
                    logger.info("Updated item: %s", json.dumps(response.get('Attributes', {}), indent=4, cls=DecimalEncoder))
                    return

            logger.info(f"Validated: Nested attribute '{key}' exists in the item. Proceeding with update.")

            # Infer the DynamoDB type for the value
            final_value = infer_dynamodb_type(value, key_parts)

            # Build the UpdateExpression dynamically for nested update
            update_expression = "SET " + ".".join([f"#k{i}" for i in range(len(key_parts))]) + " = :val"
            expression_attribute_names = {f"#k{i}": key_part for i, key_part in enumerate(key_parts)}
            expression_attribute_values = {":val": final_value}

            # Perform update for the nested field
            response = table.update_item(
                Key={partition_key: partition_value},
                UpdateExpression=update_expression,
                ExpressionAttributeNames=expression_attribute_names,
                ExpressionAttributeValues=expression_attribute_values,
                # Removed ConditionExpression to allow creating new attributes
                ReturnValues="ALL_NEW"
            )
        else:
            # Existing logic for non-nested attributes or full Map replacement
            if key not in item:
                logger.info(f"Attribute '{key}' does not exist; creating it.")
                final_value = value
                if value.startswith('{') and value.endswith('}'):
                    try:
                        final_value = json.loads(value)
                        logger.info(f"Parsed value as JSON: {final_value}")
                    except json.JSONDecodeError:
                        logger.warning(f"Value '{value}' looks like JSON but failed to parse; treating as string.")
                response = table.update_item(
                    Key={partition_key: partition_value},
                    UpdateExpression="SET #attr = :val",
                    ExpressionAttributeNames={"#attr": key},
                    ExpressionAttributeValues={":val": final_value},
                    ReturnValues="ALL_NEW"
                )
                logger.info("Attribute created and updated successfully!")
                logger.info("Updated item: %s", json.dumps(response.get('Attributes', {}), indent=4, cls=DecimalEncoder))
                return

            logger.info(f"Validated: Attribute '{key}' exists in the item. Proceeding with update.")

            # Try to parse the value as JSON if it looks like a JSON object
            final_value = value
            if value.startswith('{') and value.endswith('}'):
                try:
                    final_value = json.loads(value)
                    logger.info(f"Parsed value as JSON: {final_value}")
                except json.JSONDecodeError:
                    logger.warning(f"Value '{value}' looks like JSON but failed to parse; treating as string.")

            response = table.update_item(
                Key={partition_key: partition_value},
                UpdateExpression="SET #attr = :val",
                ExpressionAttributeNames={"#attr": key},
                ExpressionAttributeValues={":val": final_value},
                ReturnValues="ALL_NEW"
            )

        logger.info("Attribute updated successfully!")
        logger.info("Updated item: %s", json.dumps(response.get('Attributes', {}), indent=4, cls=DecimalEncoder))
    except ValueError:
        logger.error("Invalid update attribute format. Use key=value or key.nested1.nested2=value for nested updates.")
        sys.exit(1)
    except ClientError as e:
        logger.error(f"Error updating item: {e.response['Error']['Message']}")
        sys.exit(1)
    except Exception as e:
        logger.error(f"Error updating item: {e}")
        sys.exit(1)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--action", required=True, help="Action to perform: fetch or update")
    parser.add_argument("--table", required=True, help="DynamoDB table name")
    parser.add_argument("--partition-key", required=True, help="Partition key name")
    parser.add_argument("--partition-value", required=True, help="Partition key value")
    parser.add_argument("--update", help="Attribute to update in key=value format", default='')

    args = parser.parse_args()

    if args.action == "fetch":
        fetch_item(args.table, args.partition_key, args.partition_value)
    elif args.action == "update":
        if not args.update:
            logger.error("UpdateAttribute is required for the update action.")
            sys.exit(1)
        update_item(args.table, args.partition_key, args.partition_value, args.update)
    else:
        logger.error("Invalid action. Use 'fetch' or 'update'.")
        sys.exit(1)
