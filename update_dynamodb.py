import argparse
import boto3
import json
import sys
import logging
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
    """Recursively fetch a nested item using a list of keys."""
    current = item
    for key in keys:
        if not isinstance(current, dict) or key not in current:
            return None
        current = current[key]
    return current

def infer_value_type(value, existing_value):
    """Infer the type of the new value based on the existing value's type."""
    if isinstance(existing_value, Decimal):
        try:
            return Decimal(value)
        except (ValueError, TypeError):
            logger.warning(f"Value '{value}' could not be converted to a Decimal; keeping as string.")
            return value
    elif isinstance(existing_value, (list, dict)):
        # If the existing value is a List or Map, try to parse the new value as JSON
        if isinstance(value, str) and (value.startswith('{') or value.startswith('[')):
            try:
                return json.loads(value)
            except json.JSONDecodeError:
                logger.warning(f"Value '{value}' looks like JSON but failed to parse; keeping as string.")
                return value
        return value
    # Default to string or whatever the value already is
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
                logger.error(f"Top-level attribute '{attr}' does not exist in the item. Update aborted.")
                sys.exit(1)

            # Check if the full nested path exists
            nested_item = get_nested_item(item, key_parts[:-1])  # Check up to the second-to-last key
            if nested_item is None or not isinstance(nested_item, dict) or key_parts[-1] not in nested_item:
                logger.error(f"Nested attribute '{key}' does not exist in the item. Update aborted.")
                sys.exit(1)

            # Get the existing value to infer its type
            existing_value = nested_item[key_parts[-1]]
            logger.info(f"Existing value for '{key}' is of type {type(existing_value)}: {existing_value}")

            # Infer the type of the new value based on the existing value
            final_value = infer_value_type(value, existing_value)

            logger.info(f"Validated: Nested attribute '{key}' exists in the item. Proceeding with update.")

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
                ConditionExpression="attribute_exists(" + ".".join([f"#k{i}" for i in range(len(key_parts))]) + ")",
                ReturnValues="ALL_NEW"
            )
        else:
            # Logic for non-nested attributes or full Map replacement
            if key not in item:
                logger.error(f"Attribute '{key}' does not exist in the item. Update aborted.")
                sys.exit(1)

            # Get the existing value to infer its type
            existing_value = item[key]
            logger.info(f"Existing value for '{key}' is of type {type(existing_value)}: {existing_value}")

            # Infer the type of the new value based on the existing value
            final_value = infer_value_type(value, existing_value)

            logger.info(f"Validated: Attribute '{key}' exists in the item. Proceeding with update.")

            response = table.update_item(
                Key={partition_key: partition_value},
                UpdateExpression="SET #attr = :val",
                ExpressionAttributeNames={"#attr": key},
                ExpressionAttributeValues={":val": final_value},
                ConditionExpression="attribute_exists(#attr)",
                ReturnValues="ALL_NEW"
            )

        logger.info("Attribute updated successfully!")
        logger.info("Updated item: %s", json.dumps(response.get('Attributes', {}), indent=4, cls=DecimalEncoder))
    except ValueError:
        logger.error("Invalid update attribute format. Use key=value or key.nested1.nested2=value for nested updates.")
        sys.exit(1)
    except ClientError as e:
        if e.response['Error']['Code'] == 'ConditionalCheckFailedException':
            logger.error(f"Attribute '{key}' does not exist in the item. Update failed.")
        else:
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
