# Authentication System: Visual Guide & Quick Reference

## 1. PROVIDER CLASS HIERARCHY

```
┌─────────────────────────────────────────────────────────────────────┐
│                      AuthProvider (Abstract Base)                    │
│  • authenticate(username, password) → token_dict | None             │
│  • get_authorization_url(state) → str | None                        │
│  • exchange_code_for_token(code) → token_dict | None                │
│  • get_userinfo(token) → userinfo_dict | None                       │
│  • logout_url(redirect_to="/") → str                                │
└─────────────────────────────────────────────────────────────────────┘
         ▲                              ▲
         │                              │
    ┌────┴────┐                    ┌────┴────────────────────────────┐
    │          │                    │                                 │
┌───┴─────┐   │         ┌──────────┴──────────┐         ┌────────────┴──────┐
│LocalProv│   │         │  OAuthCodeFlow      │         │ KeycloakROPC      │
│ (ROPC)  │   │         │  (Abstract Base)    │         │ (Direct ROPC)     │
└─────────┘   │         │ • PKCE support      │         │                   │
              │         │ • Token refresh     │         │ ✅ ROPC           │
              │         │ • OAuth code flow   │         │ ❌ Redirect       │
              │         └──────────┬──────────┘         └───────────────────┘
              │                    │
              │     ┌──────┬───────┼───────┬──────────┐
              │     │      │       │       │          │
         ┌────┴─────┴─┐ ┌──┴─┐ ┌──┴─┐ ┌──┴──┐ ┌────┴────┐
         │ KeycloakPr │ │Entr│ │Okta│ │Auth0│ │(unused) │
         │ (OAuth)    │ │ ID │ │    │ │     │ │         │
         │            │ │    │ │    │ │     │ │         │
         │✅ Redirect │ │✅  │ │✅  │ │✅   │ │         │
         │✅ Refresh  │ │✅  │ │✅  │ │✅   │ │         │
         │❌ ROPC     │ │❌  │ │❌  │ │❌   │ │         │
         └────────────┘ └────┘ └────┘ └─────┘ └─────────┘

Legend:
  ✅ = Supported
  ❌ = Not supported
  ROPC = Resource Owner Password Credentials (username/password)
  Redirect = OAuth Authorization Code Flow with redirect
```

---

## 2. AUTHENTICATION FLOWS

### Flow A: OAuth Code Flow (Keycloak, Entra, Okta, Auth0)

```
┌─────────┐
│ Browser │
└────┬────┘
     │
     │ 1. User clicks "Login"
     │
     ↓
┌──────────────────────────────┐
│  GET /login                  │
│  • Generate PKCE verifier    │
│  • Generate PKCE challenge   │
│  • Store state in session    │
│  • Build auth URL            │
└──────────────┬───────────────┘
               │ 2. Redirect to provider
               ↓
        ┌──────────────────┐
        │ Provider Login   │
        │ (Keycloak/Entra/ │
        │  Okta/Auth0)     │
        │                  │
        │ User authenticates
        └────────┬─────────┘
                 │ 3. Callback with authorization code
                 │
                 ↓
        ┌────────────────────────────────┐
        │ GET /callback                  │
        │  ?code=...&state=...           │
        │                                │
        │ • Validate state               │
        │ • Exchange code for tokens     │
        │   (using PKCE verifier)        │
        │ • Extract roles from JWT       │
        │ • Create Flask session         │
        └────────────┬───────────────────┘
                     │ 4. Redirect to dashboard
                     ↓
            ┌─────────────────┐
            │ Dashboard       │
            │ (/almond/admin) │
            └─────────────────┘
```

**Token Exchange Details:**
```
POST {token_url}
  grant_type=authorization_code
  code={received_code}
  code_verifier={PKCE_verifier}
  client_id={client_id}
  client_secret={client_secret}
  redirect_uri={callback_uri}

Response:
{
  "access_token": "JWT...",
  "refresh_token": "JWT...",
  "id_token": "JWT...",
  "expires_in": 3600
}
```

---

### Flow B: ROPC (Resource Owner Password Credentials) - Keycloak Only

```
┌─────────┐
│ Browser │
└────┬────┘
     │
     │ 1. User submits username/password form
     │    POST /almond/admin
     │    action_type=create_session
     │    uname=xxx, psw=xxx
     │
     ↓
┌──────────────────────────────────────┐
│ ProviderFactory.create()             │
│ • Read auth_provider_name from config│
│ • Return KeycloakROPC or others      │
└──────────────┬───────────────────────┘
               │
               ↓
        ┌──────────────────────────────┐
        │ provider.authenticate()      │
        │                              │
        │ POST {token_url}             │
        │   grant_type=password        │
        │   username/password          │
        │   client_id/secret           │
        └───────────┬──────────────────┘
                    │
                    ↓
         ┌──────────────────┐
         │ Token Response   │
         │ {access_token}   │
         │ {refresh_token}  │
         └────────┬─────────┘
                  │
                  ↓
        ┌──────────────────────────────┐
        │ extract_roles_from_token()   │
        │                              │
        │ Decode JWT (no verify)       │
        │ Extract roles from:          │
        │  1. realm_access.roles       │
        │  2. resource_access.roles    │
        │  3. roles claim              │
        │  4. groups claim             │
        │  5. CUSTOM_ROLE_MAPPING      │
        │  6. Default ["admin"]        │
        └────────┬─────────────────────┘
                 │
                 ↓
        ┌──────────────────────────────┐
        │ Create Flask Session         │
        │ session["user"] = {          │
        │   username,                  │
        │   provider,                  │
        │   roles,                     │
        │   access_token,              │
        │   id_token                   │
        │ }                            │
        └────────┬─────────────────────┘
                 │ 2. Store in session
                 ↓
        ┌─────────────────┐
        │ Redirect to     │
        │ /almond/admin   │
        └─────────────────┘
```

---

### Flow C: Local File-Based Authentication

```
┌─────────┐
│ Browser │
└────┬────┘
     │
     │ 1. Submit login form
     │
     ↓
┌──────────────────────────┐
│ ProviderFactory.create() │
│ → LocalProvider()        │
│                          │
│ _load_users() reads      │
│ /etc/almond/users.conf   │
│                          │
│ JSON lines format:       │
│ {"user": "hash"}         │
│ {"user": {              │
│   "password": "hash",   │
│   "roles": ["admin"]    │
│ }}                      │
└────────────┬─────────────┘
             │
             ↓
     ┌────────────────────┐
     │ authenticate()     │
     │                    │
     │ • Get username/pwd │
     │ • Look up in users │
     │ • Verify password  │
     │   hash             │
     │ • Get roles from   │
     │   users config     │
     └────────┬───────────┘
              │
              ↓
     ┌────────────────────┐
     │ Create Session     │
     │ No tokens needed   │
     │ Roles from file    │
     └────────┬───────────┘
              │
              ↓
     ┌─────────────────┐
     │ Redirect to     │
     │ /almond/admin   │
     └─────────────────┘
```

---

## 3. ROLE EXTRACTION LOGIC (DETAILED)

```
Input: access_token (JWT string)
       provider_name (string)
       username (string)

Step 1: Decode JWT (NO signature verification)
│
├─→ Extract realm_access.roles?
│   └─→ YES: Add to roles list
│   └─→ NO: Continue
│
├─→ Extract resource_access[*].roles?
│   └─→ YES: Add to roles list (iterate all clients)
│   └─→ NO: Continue
│
├─→ Extract direct "roles" claim?
│   └─→ YES: Add to roles list
│   └─→ NO: Continue
│
├─→ Extract "groups" claim?
│   └─→ YES: Add to roles list
│   └─→ NO: Continue
│
├─→ Check CUSTOM_ROLE_MAPPING[username]?
│   └─→ YES: Use mapped roles
│   └─→ NO: Continue
│
├─→ Is provider "local"?
│   └─→ YES: Default to ["admin"]
│   └─→ NO: roles empty, log warning
│
Final Output: roles (list of strings)

Example Outputs:
  Keycloak → ["admin", "realm_admin", "almond-exec"]
  Entra → ["admin", "operator"]
  Auth0 → ["viewer"]
  Local → ["admin"] (from file)
  Custom → ["custom-role"] (from mapping)
  Unknown → [] (empty, with warning)
```

---

## 4. FLASK SESSION STRUCTURE AFTER LOGIN

```python
session = {
    "login": "true",                    # Indicates logged in
    
    "user": {
        "username": "john.doe",         # User identifier
        "provider": "keycloak",         # Which provider
        "source": "external",           # "external" or "local"
        "roles": [
            "admin",
            "almond-exec"
        ],
        "id_token": "eyJhbGc...",       # Optional ID token
        "access_token": "eyJhbGc...",   # Access token from provider
    },
    
    "tokens": {                         # For OAuth providers only
        "access_token": "eyJhbGc...",
        "refresh_token": "refresh_token_value",
        "id_token": "eyJhbGc...",
    }
}

# Access in routes:
user = session.get("user", {})
username = user.get("username")
roles = user.get("roles", [])
provider = user.get("provider")

is_admin = "admin" in roles
```

---

## 5. PROVIDER DECISION TREE

```
Application Start
│
└─→ Read config: api.authProvider
    │
    ├─→ NOT SET
    │  └─→ DEFAULT: "local"
    │
    ├─→ SET TO: "keycloak"
    │  └─→ Check: enable_login_redirect?
    │      ├─→ TRUE → KeycloakProvider (OAuth)
    │      └─→ FALSE → KeycloakROPC (ROPC)
    │
    ├─→ SET TO: "entra"
    │  └─→ EntraIDProvider (OAuth ONLY)
    │
    ├─→ SET TO: "okta"
    │  └─→ OktaProvider (OAuth ONLY)
    │
    ├─→ SET TO: "auth0"
    │  └─→ 🔴 BUG: Validation checks for "oath0" (typo)
    │      └─→ FAILS: Falls back to "local"
    │
    ├─→ SET TO: "local"
    │  └─→ LocalProvider (ROPC from file)
    │
    └─→ SET TO: ANYTHING ELSE
       └─→ 🔴 INVALID: Falls back to "local"

RESULT: set_provider(instance)
        ↓
    Store in global module variable
        ↓
    Access via get_provider() throughout app
```

---

## 6. URL ENDPOINTS MAPPED TO PROVIDERS

### Keycloak Endpoints

**OAuth Flow:**
```
Frontend:
  Auth URL: http://kc-frontend:8089/realms/almondmonitor/protocol/openid-connect/auth
  Logout:   http://kc-frontend:8089/realms/almondmonitor/protocol/openid-connect/logout

Backend (API):
  Token URL: http://kc-backend:8089/realms/almondmonitor/protocol/openid-connect/token
  Userinfo:  http://kc-backend:8089/realms/almondmonitor/protocol/openid-connect/userinfo
```

**ROPC Flow:**
```
Token URL: {KEYCLOAK_TOKEN_URL}
Userinfo:  {KEYCLOAK_USERINFO_URL}
```

---

### Entra ID Endpoints

```
Auth URL:  https://login.microsoftonline.com/{tenant_id}/oauth2/v2.0/authorize
Token URL: https://login.microsoftonline.com/{tenant_id}/oauth2/v2.0/token
Logout:    https://login.microsoftonline.com/{tenant_id}/oauth2/v2.0/logout
```

---

### Okta Endpoints

```
Auth URL:  https://{domain}/oauth2/default/v1/authorize
Token URL: https://{domain}/oauth2/default/v1/token
Logout:    https://{domain}/oauth2/default/v1/logout
```

---

### Auth0 Endpoints

```
Auth URL:  https://{domain}/authorize
Token URL: https://{domain}/oauth/token
Logout:    https://{domain}/v2/logout
```

---

### Local Authentication

```
No remote endpoints
Uses file: /etc/almond/users.conf
```

---

## 7. ROLE CLAIM NAMES BY PROVIDER

```
┌────────────┬──────────────────────────┬──────────────────────────┐
│ Provider   │ Primary Role Claim       │ Fallback Claim           │
├────────────┼──────────────────────────┼──────────────────────────┤
│ Keycloak   │ realm_access.roles       │ resource_access[*].roles │
│            │ (STANDARD)               │ (Client-specific)        │
├────────────┼──────────────────────────┼──────────────────────────┤
│ Entra      │ roles                    │ groups                   │
│            │ (Optional - configured)  │ (Directory groups)       │
├────────────┼──────────────────────────┼──────────────────────────┤
│ Okta       │ roles                    │ groups                   │
│            │ (Optional - configured)  │ (Okta groups)            │
├────────────┼──────────────────────────┼──────────────────────────┤
│ Auth0      │ https://example/roles    │ groups                   │
│            │ (Custom claim - config)  │ (Auth0 groups)           │
├────────────┼──────────────────────────┼──────────────────────────┤
│ Local      │ roles (from JSON)        │ N/A                      │
│            │ (File-based)             │                          │
└────────────┴──────────────────────────┴──────────────────────────┘

⚠️  IMPORTANT:
  • Keycloak stores roles in NESTED structures (realm_access, resource_access)
  • Other providers use DIRECT claims (roles, groups)
  • Custom role mappings bypass all this
```

---

## 8. KNOWN ISSUES & BUGS

### 🔴 CRITICAL

**Auth0 Blocked by Typo**
```
File: admin_page.py, line 415

Current:
  if not auth_provider_name.lower() in ["keycloak", "entra", "oath0", "okta"]:

Should be:
  if not auth_provider_name.lower() in ["keycloak", "entra", "auth0", "okta"]:
                                                               ^^^^^
                                                          (not "oath0")

Effect: Auth0 provider selection FAILS → Falls back to "local"
```

---

### 🟡 MEDIUM

**Missing Entra get_scope() Method**
```
File: entra.py, line 29

Current:
  def getScope(self):  # WRONG: lowercase 's'
      return "openid offline_access"

Issue: Base class calls get_scope() (lowercase 's')
       Method name doesn't match → Scope not set

Should be:
  def get_scope(self):  # Correct method name
      return "openid offline_access"
```

---

**Keycloak-Centric Role Extraction**
```
File: login_handler.py, lines 55-74

Issue: First checks for Keycloak-specific structures
       (realm_access, resource_access) before generic claims

Impact: If provider uses direct "roles" claim,
        code still checks Keycloak structure first
        (inefficient but works due to fallback)

Recommended: Check provider type first, then structure
```

---

### 🟢 LOW

**Hardcoded Default Provider**
```
File: auth_config.py, line 10

Current:
  AUTH_PROVIDER_NAME = "keycloak"

Impact: Low - easily overridden by config file

Recommended: Use environment variable instead
```

---

## 9. TESTING CHECKLIST

```
[ ] Keycloak ROPC
    [ ] Login with username/password
    [ ] Roles extracted from access_token
    [ ] Session created with roles

[ ] Keycloak OAuth
    [ ] Redirect to Keycloak login
    [ ] Callback with code
    [ ] Token exchange successful
    [ ] Roles extracted (realm_access + resource_access)

[ ] Entra ID
    [ ] Redirect to Azure login
    [ ] Callback with code
    [ ] Token exchange successful
    [ ] Roles from "roles" or "groups" claim

[ ] Okta
    [ ] Redirect to Okta login
    [ ] Callback with code
    [ ] Token exchange successful
    [ ] Roles from "roles" claim

[ ] Auth0
    [ ] Fix auth0 typo first!
    [ ] Redirect to Auth0 login
    [ ] Callback with code
    [ ] Token exchange successful
    [ ] Roles from custom claims

[ ] Local
    [ ] Login with local user
    [ ] Roles from /etc/almond/users.conf
    [ ] Password verification works

[ ] Custom Role Mapping
    [ ] User without roles in token
    [ ] Gets assigned roles from CUSTOM_ROLE_MAPPING
    [ ] Verification appears in logs
```

---

## 10. QUICK FIXES REQUIRED

### Fix #1: Auth0 Typo (URGENT)

**File:** `admin_page.py:415`

```python
# BEFORE:
if not auth_provider_name.lower() in ["keycloak", "entra", "oath0", "okta"]:

# AFTER:
if not auth_provider_name.lower() in ["keycloak", "entra", "auth0", "okta"]:
```

---

### Fix #2: Entra Scope Method

**File:** `entra.py:29`

```python
# BEFORE:
def getScope(self):
    return "openid offline_access"

# AFTER:
def get_scope(self):
    return "openid offline_access"
```

---

### Fix #3: Add Missing Entra Scope Method (if not inherited)

**File:** `entra.py`

Add to EntraIDProvider class:
```python
def get_scope(self):
    return "openid offline_access"
```

---

## Document Version
- **Version:** 1.0
- **Type:** Quick Reference & Visual Guide
- **Last Updated:** April 20, 2026
