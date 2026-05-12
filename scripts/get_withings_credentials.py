#!/usr/bin/env python3
"""
Get Withings API credentials for the e-ink display.

This script helps you obtain the necessary credentials to fetch
your weight data from Withings API.

Requirements:
    pip install requests
"""

import requests
import webbrowser
import urllib.parse
from http.server import HTTPServer, BaseHTTPRequestHandler
import threading
import json

print("=" * 60)
print("Withings API Credential Setup")
print("=" * 60)
print()
print("First, create an app at: https://developer.withings.com/dashboard/")
print()
print("Enter your credentials:")
print()

client_id = input("Client ID: ").strip()
client_secret = input("Client Secret: ").strip()

if not client_id or not client_secret:
    print("Error: Client ID and Secret are required")
    exit(1)

REDIRECT_URI = "http://localhost:8080/callback"

# Step 1: Get authorization code
auth_url = "https://account.withings.com/oauth2_user/authorize2"
params = {
    "response_type": "code",
    "client_id": client_id,
    "scope": "user.info,user.metrics,user.activity",
    "redirect_uri": REDIRECT_URI,
    "state": "esp32_weight_display",
}

auth_url_with_params = f"{auth_url}?{urllib.parse.urlencode(params)}"

print()
print("Opening browser for authorization...")
print()

authorization_code = None

class CallbackHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        global authorization_code
        if self.path.startswith('/callback'):
            query = urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query)
            authorization_code = query.get('code', [None])[0]
            
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            self.wfile.write(b"<html><body><h1>Success!</h1><p>You can close this window.</p></body></html>")
            threading.Thread(target=self.server.shutdown).start()
        else:
            self.send_response(404)
            self.end_headers()

webbrowser.open(auth_url_with_params)

server = HTTPServer(('localhost', 8080), CallbackHandler)
server.serve_forever()

if not authorization_code:
    print("Error: No authorization code received")
    exit(1)

# Step 2: Exchange code for tokens
token_url = "https://wbsapi.withings.net/v2/oauth2"
data = {
    "action": "requesttoken",
    "grant_type": "authorization_code",
    "client_id": client_id,
    "client_secret": client_secret,
    "code": authorization_code,
    "redirect_uri": REDIRECT_URI,
}

response = requests.post(token_url, data=data)
token_data = response.json()

if response.status_code != 200 or token_data.get("status") != 0:
    print(f"Error getting tokens: {token_data}")
    exit(1)

body = token_data.get("body", {})
access_token = body.get("access_token", "")
refresh_token = body.get("refresh_token", "")
user_id = body.get("userid", "")

print()
print("=" * 60)
print("Your Withings Credentials:")
print("=" * 60)
print()
print(f"WITHINGS_CLIENT_ID = \"{client_id}\"")
print(f"WITHINGS_CLIENT_SECRET = \"{client_secret}\"")
print(f"WITHINGS_ACCESS_TOKEN = \"{access_token}\"")
print(f"WITHINGS_REFRESH_TOKEN = \"{refresh_token}\"")
print(f"WITHINGS_USER_ID = \"{user_id}\"")
print()
print("Copy these to src/credentials.h")
print("=" * 60)
