import argparse
import boto3
from botocore.exceptions import ClientError
import logging
import sys

# Set up logging for pipeline
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger()

# Suppress botocore and boto3 INFO logs to remove credential messages
logging.getLogger('botocore').setLevel(logging.WARNING)
logging.getLogger('boto3').setLevel(logging.WARNING)

def fetch_attributes(table_name, partition_key, partition_value):
    dynamodb = boto3.resource('dynamodb', region_name='ap-south-1')
    table = dynamodb.Table(table_name)

    try:
        logger.info(f"Fetching item from table: {table_name} with key: {partition_key}={partition_value}")
        response = table.get_item(
            Key={
                partition_key: partition_value
            }
        )
    except ClientError as e:
        logger.error(f"Error fetching item: {e.response['Error']['Message']}")
        sys.exit(1)

    item = response.get('Item')
    if not item:
        logger.error("No item found with the given key.")
        sys.exit(1)

    print("Attributes:")
    for k, v in item.items():
        print(f"{k} = {v}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Fetch DynamoDB item attributes.')
    parser.add_argument('--table', required=True, help='DynamoDB table name')
    parser.add_argument('--partition-key', required=True, help='Partition key name')
    parser.add_argument('--partition-value', required=True, help='Partition key value')

    args = parser.parse_args()
    fetch_attributes(args.table, args.partition_key, args.partition_value)