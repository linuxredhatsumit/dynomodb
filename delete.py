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

def delete_item(table_name, partition_key, partition_value):
    dynamodb = boto3.resource('dynamodb', region_name='ap-south-1')
    table = dynamodb.Table(table_name)
    try:
        logger.info(f"Attempting to delete item from table: {table_name} with key: {partition_key}={partition_value}")
        response = table.delete_item(
            Key={
                partition_key: partition_value
            },
            ReturnValues="ALL_OLD"  # Return the deleted item (if it existed)
        )
        deleted_item = response.get('Attributes', None)
        if deleted_item:
            logger.info("Item deleted successfully!")
            logger.info("Deleted item: %s", json.dumps(deleted_item, indent=4, cls=DecimalEncoder))
        else:
            logger.info("Item deleted successfully, but no item was found with the given key (already deleted or never existed).")
    except ClientError as e:
        logger.error(f"Error deleting item: {e.response['Error']['Message']}")
        sys.exit(1)
    except Exception as e:
        logger.error(f"Error deleting item: {e}")
        sys.exit(1)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Delete DynamoDB item.')
    parser.add_argument('--table', required=True, help='DynamoDB table name')
    parser.add_argument('--partition-key', required=True, help='Partition key name')
    parser.add_argument('--partition-value', required=True, help='Partition key value')

    args = parser.parse_args()
    delete_item(args.table, args.partition_key, args.partition_value)
