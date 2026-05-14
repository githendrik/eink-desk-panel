#!/usr/bin/env python3
"""
Get Strava API credentials for the e-ink display.

This script helps you obtain the necessary credentials to fetch
your activity data from the Strava API.

Requirements:
    pip install requests

Setup:
    1. Go to https://www.strava.com/settings/api
    2. Create an application
    3. Set "Authorization Callback Domain" to "localhost"
    4. Run this script with your Client ID and Client Secret
"""

import requests
import webbrowser
import urllib.parse
from http.server import HTTPServer, BaseHTTPRequestHandler
import threading

print("=" * 60)
print("Strava API Credential Setup")
print("=" * 60)
print()
print("First, create an app at: https://www.strava.com/settings/api")
print("Set 'Authorization Callback Domain' to: localhost")
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
auth_url = "https://www.strava.com/oauth/authorize"
params = {
    "client_id": client_id,
    "response_type": "code",
    "redirect_uri": REDIRECT_URI,
    "scope": "activity:read_all",
    "approval_prompt": "auto",
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
token_url = "https://www.strava.com/oauth/token"
data = {
    "client_id": client_id,
    "client_secret": client_secret,
    "code": authorization_code,
    "grant_type": "authorization_code",
}

response = requests.post(token_url, data=data)
token_data = response.json()

if response.status_code != 200 or "access_token" not in token_data:
    print(f"Error getting tokens: {token_data}")
    exit(1)

access_token = token_data.get("access_token", "")
refresh_token = token_data.get("refresh_token", "")

print()
print("=" * 60)
print("Your Strava Credentials:")
print("=" * 60)
print()
print(f'#define STRAVA_CLIENT_ID "{client_id}"')
print(f'#define STRAVA_CLIENT_SECRET "{client_secret}"')
print(f'#define STRAVA_ACCESS_TOKEN "{access_token}"')
print(f'#define STRAVA_REFRESH_TOKEN "{refresh_token}"')
print()
print("Copy these to src/credentials.h")
print("=" * 60)
