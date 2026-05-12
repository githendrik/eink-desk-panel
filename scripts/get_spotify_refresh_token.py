#!/usr/bin/env python3
"""
Spotify Refresh Token Generator
================================
This script helps you generate a refresh token for your ESP32 to access Spotify.

Prerequisites:
1. Go to https://developer.spotify.com/dashboard
2. Create an app (or use existing one)
3. In app settings, add redirect URI: http://localhost:8888/callback
4. Note your Client ID and Client Secret

Usage:
    python3 get_spotify_refresh_token.py
"""

import base64
import json
import urllib.parse
import urllib.request
import webbrowser
from http.server import BaseHTTPRequestHandler, HTTPServer
import sys

# Configuration - You'll be prompted to enter these
CLIENT_ID = ""
CLIENT_SECRET = ""
REDIRECT_URI = "http://127.0.0.1:8888/callback"
SCOPES = "user-read-currently-playing user-read-playback-state"

# Global variable to store the authorization code
auth_code = None


class CallbackHandler(BaseHTTPRequestHandler):
    """Handle the OAuth callback from Spotify"""
    
    def do_GET(self):
        global auth_code
        
        # Parse the query parameters
        query = urllib.parse.urlparse(self.path).query
        params = urllib.parse.parse_qs(query)
        
        if 'code' in params:
            auth_code = params['code'][0]
            
            # Send success response
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            self.wfile.write(b"""
                <html>
                <body>
                    <h1>Success!</h1>
                    <p>Authorization successful! You can close this window and return to the terminal.</p>
                </body>
                </html>
            """)
        else:
            # Send error response
            self.send_response(400)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            self.wfile.write(b"""
                <html>
                <body>
                    <h1>Error</h1>
                    <p>Authorization failed. Please try again.</p>
                </body>
                </html>
            """)
    
    def log_message(self, format, *args):
        """Suppress default logging"""
        pass


def get_authorization_code():
    """Open browser for user authorization and start local server to receive callback"""
    
    # Build authorization URL
    auth_params = {
        'client_id': CLIENT_ID,
        'response_type': 'code',
        'redirect_uri': REDIRECT_URI,
        'scope': SCOPES
    }
    
    auth_url = 'https://accounts.spotify.com/authorize?' + urllib.parse.urlencode(auth_params)
    
    print("\n🎵 Opening browser for Spotify authorization...")
    print(f"If browser doesn't open, visit this URL manually:\n{auth_url}\n")
    
    # Open browser
    webbrowser.open(auth_url)
    
    # Start local server to receive callback
    server = HTTPServer(('localhost', 8888), CallbackHandler)
    print("⏳ Waiting for authorization... (authorize in your browser)")
    
    # Wait for one request (the callback)
    server.handle_request()
    server.server_close()
    
    return auth_code


def get_refresh_token(auth_code):
    """Exchange authorization code for refresh token"""
    
    # Prepare the token request
    token_url = 'https://accounts.spotify.com/api/token'
    
    # Create base64 encoded authorization header
    auth_str = f"{CLIENT_ID}:{CLIENT_SECRET}"
    auth_bytes = auth_str.encode('ascii')
    auth_b64 = base64.b64encode(auth_bytes).decode('ascii')
    
    headers = {
        'Authorization': f'Basic {auth_b64}',
        'Content-Type': 'application/x-www-form-urlencoded'
    }
    
    data = {
        'grant_type': 'authorization_code',
        'code': auth_code,
        'redirect_uri': REDIRECT_URI
    }
    
    # Make the request
    request = urllib.request.Request(
        token_url,
        data=urllib.parse.urlencode(data).encode('utf-8'),
        headers=headers
    )
    
    try:
        response = urllib.request.urlopen(request)
        response_data = json.loads(response.read().decode('utf-8'))
        
        return response_data.get('refresh_token'), response_data.get('access_token')
    
    except urllib.error.HTTPError as e:
        error_body = e.read().decode('utf-8')
        print(f"❌ Error getting tokens: {e.code}")
        print(error_body)
        return None, None


def main():
    global CLIENT_ID, CLIENT_SECRET
    
    print("=" * 60)
    print("🎵  Spotify Refresh Token Generator for ESP32")
    print("=" * 60)
    
    print("\n📋 Setup Instructions:")
    print("1. Go to: https://developer.spotify.com/dashboard")
    print("2. Create an app (or select existing)")
    print("3. Click 'Edit Settings'")
    print("4. Add Redirect URI: http://localhost:8888/callback")
    print("5. Save settings")
    print("\n")
    
    # Get credentials from user
    CLIENT_ID = input("Enter your Spotify Client ID: ").strip()
    CLIENT_SECRET = input("Enter your Spotify Client Secret: ").strip()
    
    if not CLIENT_ID or not CLIENT_SECRET:
        print("❌ Client ID and Secret are required!")
        sys.exit(1)
    
    # Get authorization code
    code = get_authorization_code()
    
    if not code:
        print("❌ Failed to get authorization code")
        sys.exit(1)
    
    print("\n✅ Authorization code received!")
    
    # Exchange for refresh token
    print("🔄 Exchanging code for refresh token...")
    refresh_token, access_token = get_refresh_token(code)
    
    if not refresh_token:
        print("❌ Failed to get refresh token")
        sys.exit(1)
    
    # Display results
    print("\n" + "=" * 60)
    print("✅ SUCCESS! Your tokens:")
    print("=" * 60)
    print(f"\nClient ID:\n{CLIENT_ID}")
    print(f"\nClient Secret:\n{CLIENT_SECRET}")
    print(f"\n🔑 Refresh Token (save this in credentials.h):\n{refresh_token}")
    print(f"\n📝 Access Token (for testing - expires in 1 hour):\n{access_token}")
    print("\n" + "=" * 60)
    print("\n📝 Next Steps:")
    print("1. Copy your credentials.h.example to credentials.h (if not already done)")
    print("2. Add these values to your credentials.h file:")
    print(f'   SPOTIFY_CLIENT_ID = "{CLIENT_ID}"')
    print(f'   SPOTIFY_CLIENT_SECRET = "{CLIENT_SECRET}"')
    print(f'   SPOTIFY_REFRESH_TOKEN = "{refresh_token}"')
    print("3. Upload to your ESP32")
    print("4. Your ESP32 will now be able to see what you're playing!")
    print("\n✨ The refresh token doesn't expire, so this is a one-time setup!")
    print("=" * 60 + "\n")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n❌ Cancelled by user")
        sys.exit(0)
    except Exception as e:
        print(f"\n❌ Error: {e}")
        sys.exit(1)
