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
        
        # Check if the key exists in the item
        if key not in item:
            logger.error(f"Attribute '{key}' does not exist in the item. Update aborted.")
            sys.exit(1)
        
        logger.info(f"Validated: Attribute '{key}' exists in the item. Proceeding with update.")
        
        # Perform update with condition to ensure key exists
        response = table.update_item(
            Key={partition_key: partition_value},
            UpdateExpression="SET #attr = :val",
            ExpressionAttributeNames={"#attr": key},
            ExpressionAttributeValues={":val": value},
            ConditionExpression="attribute_exists(#attr)",
            ReturnValues="ALL_NEW"
        )
        logger.info("Attribute updated successfully!")
        logger.info("Updated item: %s", json.dumps(response.get('Attributes', {}), indent=4, cls=DecimalEncoder))
    except ValueError:
        logger.error("Invalid update attribute format. Use key=value.")
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
