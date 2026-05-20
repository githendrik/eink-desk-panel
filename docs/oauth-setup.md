# OAuth Setup (Withings & Strava)

## Withings API

### Step 1: Create Developer Account

1. Go to https://developer.withings.com/dashboard/
2. Sign up or log in with your Withings account
3. Click "Create an app"
4. Fill in:
   - **App name**: e.g., "E-Ink Weight Display"
   - **Description**: Personal weight tracking
   - **Website**: `http://localhost`
   - **Redirect URI**: `http://localhost:8080/callback`
5. Select "Public API"
6. Save and get your **Client ID** and **Client Secret**

### Step 2: Run the Credential Script

```bash
cd scripts
pip3 install requests
python3 get_withings_credentials.py
```

Follow the prompts:
1. Enter your Client ID and Secret
2. Browser will open — authorize the app
3. Script will display your credentials

### Step 3: Configure the Device

Option A — via web dashboard (recommended):
1. Connect to `http://eink-panel.local`
2. Enter Withings credentials in the dashboard form

Option B — via `credentials.h` (first flash only):
```cpp
#define WITHINGS_CLIENT_ID "your_client_id"
#define WITHINGS_CLIENT_SECRET "your_client_secret"
#define WITHINGS_ACCESS_TOKEN "your_access_token"
#define WITHINGS_REFRESH_TOKEN "your_refresh_token"
#define WITHINGS_USER_ID "your_user_id"
```

### Token Lifecycle

- **Access token**: Expires after ~3 months
- **Refresh token**: Long-lived but **single-use** — each refresh returns a new one
- Tokens are persisted to **NVS flash** on the ESP32, surviving reboots
- On boot, tokens load from NVS first, falling back to `credentials.h`
- You should only need to re-run the OAuth script if NVS gets wiped or the refresh token chain breaks

### Important API Details

- Token endpoint: `POST https://wbsapi.withings.net/v2/oauth2` with `action=requesttoken`
  - Do NOT use `oauth2.withings.com` (DNS doesn't resolve on ESP32)
- Weight endpoint: `POST https://wbsapi.withings.net/measure` with `action=getmeas`
  - Do NOT include `userid` parameter
  - Do NOT use `/v2/measure` endpoint
- Weight value is returned as integer (e.g., 82300 = 82.3 kg)

### Troubleshooting

**"invalid refresh_token" (status 503)**
The refresh token has been invalidated (used once and not saved, or chain broken).
Re-run `python3 scripts/get_withings_credentials.py` to get fresh tokens.

**"invalid_token" (status 401)**
Access token expired. The device will automatically try to refresh it.
If refresh also fails, see above.

**Weight shows "--.- kg / err"**
Check serial output for the specific error. Common causes:
- Token expired and refresh failed
- Network/DNS issues on ESP32
- No weight measurements in the last 6 months

**Weight shows "STALE"**
No weight measurement in the last 7 days. Step on the scale.

---

## Strava API

### Step 1: Create Strava App

1. Go to https://www.strava.com/settings/api
2. Create an application
3. Set **Authorization Callback Domain** to `localhost`
4. Get your **Client ID** and **Client Secret**

### Step 2: Run the Credential Script

```bash
cd scripts
pip3 install requests
python3 get_strava_credentials.py
```

### Step 3: Configure the Device

Option A — via web dashboard (recommended):
1. Connect to `http://eink-panel.local`
2. Enter Strava credentials in the dashboard form

Option B — via `credentials.h`:
```cpp
#define STRAVA_CLIENT_ID "your_client_id"
#define STRAVA_CLIENT_SECRET "your_client_secret"
#define STRAVA_ACCESS_TOKEN "your_access_token"
#define STRAVA_REFRESH_TOKEN "your_refresh_token"
```

### Token Lifecycle

- **Access token**: Expires every **6 hours**
- **Refresh token**: Long-lived but **single-use**
- Same NVS persistence pattern as Withings

### Display

Shows last activity in bottom-right (toggle with rocker switch):
- Format: `Run 5.2km` with date below (e.g. `12 May`)
- Activity types: Run, Ride, Swim, Walk, Hike, MTB, Gravel, Trail, Zwift, VRun, Gym, Yoga
