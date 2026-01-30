#!/usr/bin/env python3
"""
Simple program that connects to Couchbase Analytics WITHOUT SSL
Using Couchbase Python SDK 4.x
"""

from couchbase.cluster import Cluster
from couchbase.options import ClusterOptions, AnalyticsOptions
from couchbase.auth import PasswordAuthenticator
import json

def main():
    # connection_string = "couchbase://Couchbase-EA-NLB-ccdc0c52e93096c8.elb.ap-southeast-2.amazonaws.com:8091"
    # Try using http:// prefix for HTTP bootstrap
    connection_string = "http://Couchbase-EA-NLB-ccdc0c52e93096c8.elb.ap-southeast-2.amazonaws.com:8091"

    print("Enter the username: ", end="")
    username = input().strip()

    print("Enter the password: ", end="")
    password = input().strip()

    print("------------------------------------------------")
    print(f"LOG: Connection String -> {connection_string}")
    print("------------------------------------------------")

    try:
        # Create cluster with authentication
        auth = PasswordAuthenticator(username, password)
        cluster = Cluster(connection_string, ClusterOptions(auth))

        # Wait for connection to be ready
        cluster.wait_until_ready(5.0)
        print("✅ Connected to Cluster!")

        # Execute analytics query
        query = "SELECT * FROM `travel-sample`.inventory.landmark LIMIT 1"
        print(f"Executing: {query}...")

        # Execute analytics query
        result = cluster.analytics_query(query)

        # Process and print results
        row_count = 0
        for row in result.rows():
            row_count += 1
            print(f"Result {row_count}: {json.dumps(row, indent=2)}")

        # Print metadata
        metadata = result.metadata()
        print(f"✅ Query Completed.")
        print(f"Status: {metadata.status()}")
        if metadata.metrics():
            metrics = metadata.metrics()
            print(f"Execution Time: {metrics.execution_time()}")
            print(f"Result Count: {metrics.result_count()}")

    except Exception as e:
        print(f"❌ Error: {e}")
        import traceback
        traceback.print_exc()
        return 1

    return 0

if __name__ == "__main__":
    exit(main())