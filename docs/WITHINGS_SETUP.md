# Withings API Setup

## Get Your Credentials

### Step 1: Create Developer Account

1. Go to https://developer.withings.com/dashboard/
2. Sign up or log in with your Withings account
3. Click "Create an app" or go to your dashboard
4. Fill in:
   - **App name**: e.g., "E-Ink Weight Display"
   - **Description**: Personal weight tracking
   - **Website**: Can be `http://localhost`
   - **Redirect URI**: `http://localhost:8080/callback`
5. Select "Public API" (no contract needed)
6. Save and get your **Client ID** and **Client Secret**

### Step 2: Run the Credential Script

```bash
cd scripts
pip3 install requests
python3 get_withings_credentials.py
```

Follow the prompts:
1. Enter your Client ID and Secret
2. Browser will open - authorize the app
3. Script will display your credentials

### Step 3: Update credentials.h

Copy the output to `src/credentials.h`:

```cpp
const char* WITHINGS_CLIENT_ID = "your_client_id";
const char* WITHINGS_CLIENT_SECRET = "your_client_secret";
const char* WITHINGS_ACCESS_TOKEN = "your_access_token";
const char* WITHINGS_REFRESH_TOKEN = "your_refresh_token";
const char* WITHINGS_USER_ID = "your_user_id";
```

### Step 4: Upload to ESP32

```bash
cd ..
pio run -t upload
```

## Display Layout

Your e-ink display will show:

```
┌─────────────────────────┐
│       23°               │  <- Temperature
│                         │
│  Pollen moderate        │  <- Air quality
│  Weight 75.2 kg (stable)│  <- Your weight + trend
│  Aare 15°               │  <- River temp
└─────────────────────────┘
```

## Data Refresh

- **Weather/Pollen/Aare**: Every hour
- **Weight**: Every 6 hours (to avoid API rate limits)

## Troubleshooting

### "Withings API error: 401"
- Token expired - the code automatically refreshes it
- Check that credentials are correct

### "Withings API error: 404"
- Check User ID is correct
- Verify you have a Withings scale connected to your account

### No weight data showing
- Make sure your scale has synced recently
- Check that you have at least one weight measurement in your Withings account
