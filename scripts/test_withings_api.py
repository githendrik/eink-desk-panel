#!/usr/bin/env python3
"""Test Withings API credentials"""

import requests
import sys

# Read credentials from credentials.h
credentials = {}
with open('../src/credentials.h', 'r') as f:
    for line in f:
        if 'WITHINGS_' in line and '=' in line:
            parts = line.split('=')
            if len(parts) == 2:
                key = parts[0].strip().replace('const char* ', '').replace(';', '')
                value = parts[1].strip().strip('"')
                credentials[key] = value

print("Credentials found:")
for k, v in credentials.items():
    masked = v[:10] + "..." if len(v) > 10 else v
    print(f"  {k}: {masked}")
print()

access_token = credentials.get('WITHINGS_ACCESS_TOKEN', '')
user_id = credentials.get('WITHINGS_USER_ID', '')

if not access_token or not user_id:
    print("ERROR: Missing access token or user ID")
    sys.exit(1)

# Test API call
import time
now = int(time.time())
six_months_ago = now - (180 * 24 * 60 * 60)

url = "https://wbsapi.withings.net/v2/measure"
params = {
    "action": "getactivity",
    "startdate": six_months_ago,
    "enddate": now,
    "userid": user_id,
}

headers = {
    "Authorization": f"Bearer {access_token}"
}

print(f"Calling Withings API...")
print(f"URL: {url}")
print(f"User ID: {user_id}")
print()

response = requests.get(url, params=params, headers=headers)

print(f"Status Code: {response.status_code}")
print(f"Response:")
try:
    data = response.json()
    import json
    print(json.dumps(data, indent=2))
    
    if data.get("status") == 0 and "body" in data:
        activities = data["body"].get("activities", [])
        if activities:
            print()
            print(f"✓ Success! Found {len(activities)} weight measurements")
            latest = activities[0]
            print(f"Latest weight: {latest.get('weight')} kg")
        else:
            print()
            print("✗ No weight activities found")
    else:
        print()
        print(f"✗ API Error: {data.get('error')}")
except Exception as e:
    print(f"Error parsing response: {e}")
    print(response.text)
