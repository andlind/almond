# Authentication System Analysis

## Overview

This document provides a complete breakdown of the authentication system architecture, including all provider implementations, role extraction mechanisms, provider selection logic, and Keycloak-specific dependencies.

---

## 1. AUTH PROVIDER ARCHITECTURE

### 1.1 Provider Hierarchy

```
AuthProvider (base.py) [Abstract Base]
├── KeycloakROPC (keycloak_ropc.py) - Direct ROPC to Keycloak
├── OAuthCodeFlow (oauth_code.py) [Abstract OAuth2 Code Flow]
│   ├── KeycloakProvider (keycloak.py)
│   ├── EntraIDProvider (entra.py)
│   ├── OktaProvider (okta.py)
│   └── Auth0Provider (auth0.py)
└── LocalProvider (local.py) - File-based authentication
```

### 1.2 Provider Implementations

#### **AuthProvider Base Class** (`auth/base.py`)

Defines the contract all providers must follow:

```python
class AuthProvider:
    # ROPC authentication (username/password)
    def authenticate(self, **kwargs) -> dict | None
    
    # Redirect-based OAuth flow
    def get_authorization_url(self, state=None) -> str | None
    def exchange_code_for_token(self, code) -> dict | None
    
    # User profile lookup
    def get_userinfo(self, token) -> dict | None
    
    # Logout handling
    def logout_url(self, redirect_to="/") -> str
```

---

#### **KeycloakROPC** (`auth/keycloak_ropc.py`)

**Use case:** Direct password authentication to Keycloak (Resource Owner Password Credentials)

**Capabilities:**
- ✅ ROPC (`authenticate()`) - Direct username/password login
- ❌ Redirect-based OAuth - Not supported
- ✅ Userinfo endpoint - Can retrieve user details via Bearer token

**Key Methods:**
```python
def authenticate(self, username, password):
    # POST to {token_url} with grant_type="password"
    # Returns: {"access_token", "refresh_token", "expires_in", ...}

def get_userinfo(self, token):
    # GET {userinfo_url} with Bearer token
    # Returns: User claims from Keycloak
```

**Configuration:**
```
KEYCLOAK_TOKEN_URL          - Token endpoint (direct to backend)
KEYCLOAK_USERINFO_URL       - Userinfo endpoint
KEYCLOAK_CLIENT_ID          - Client ID
KEYCLOAK_CLIENT_SECRET      - Client secret
```

---

#### **OAuthCodeFlow** (`auth/oauth_code.py`)

**Abstract base class** for all OAuth2 Authorization Code Flow providers.

**Capabilities:**
- ✅ Redirect-based OAuth - `get_authorization_url()`, `exchange_code_for_token()`
- ✅ PKCE support - Automatic code challenge/verifier generation
- ✅ Token refresh - `refresh(refresh_token)`

**Key Methods:**
```python
def get_authorization_url(self, state, challenge):
    # Returns: OAuth authorization URL with PKCE challenge
    # Client redirects to this URL

def exchange_code_for_token(self, code, verifier):
    # Exchanges authorization code + PKCE verifier for tokens
    # Returns: {"access_token", "refresh_token", "id_token", ...}

def get_scope(self):
    # Override in subclasses to define OAuth scopes
    # Base: "openid"
```

**PKCE Flow:**
1. Generate random `verifier` and SHA256 hash `challenge`
2. Build auth URL with `code_challenge` and `code_challenge_method=S256`
3. Exchange code + `code_verifier` for tokens (prevents code interception)

---

#### **KeycloakProvider** (`auth/keycloak.py`)

**Extends:** `OAuthCodeFlow`

**Use case:** OAuth2 redirect-based Keycloak login

**URL Construction:**
```python
auth_url = "{frontend_base}/realms/{realm}/protocol/openid-connect/auth"
token_url = "{backend_base}/realms/{realm}/protocol/openid-connect/token"
```

**Custom Methods:**
```python
def get_scope(self):
    return "openid offline_access"  # Enable refresh tokens

def logout_url(self, redirect_to="/", id_token=None):
    # Returns Keycloak logout endpoint
    # {frontend_base}/realms/{realm}/protocol/openid-connect/logout
    # with id_token_hint and post_logout_redirect_uri
```

**Configuration:**
```
PROVIDER_FRONTEND_BASE_URL  - Keycloak frontend (public URL)
PROVIDER_BACKEND_BASE_URL   - Keycloak backend (internal/Docker URL)
PROVIDER_REALM              - Realm name
PROVIDER_CLIENT_ID          - OAuth client ID
PROVIDER_CLIENT_SECRET      - OAuth client secret
PROVIDER_REDIRECT_URI       - Callback URL
```

---

#### **EntraIDProvider** (`auth/entra.py`)

**Extends:** `OAuthCodeFlow`

**Use case:** Azure Entra ID (Azure AD) authentication

**Endpoints:**
```python
auth_url = f"https://login.microsoftonline.com/{tenant_id}/oauth2/v2.0/authorize"
token_url = f"https://login.microsoftonline.com/{tenant_id}/oauth2/v2.0/token"
logout_url = f"https://login.microsoftonline.com/{tenant_id}/oauth2/v2.0/logout"
```

**Configuration:**
```
PROVIDER_TENANT_ID          - Azure tenant ID
PROVIDER_CLIENT_ID          - App registration client ID
PROVIDER_CLIENT_SECRET      - App registration secret
PROVIDER_REDIRECT_URI       - Callback URL
```

**Known Issue:** `getScope()` method (typo - not `get_scope()`) - may not be called correctly

---

#### **OktaProvider** (`auth/okta.py`)

**Extends:** `OAuthCodeFlow`

**Use case:** Okta identity platform

**Endpoints:**
```python
auth_url = f"https://{domain}/oauth2/default/v1/authorize"
token_url = f"https://{domain}/oauth2/default/v1/token"
logout_url = f"https://{domain}/oauth2/default/v1/logout"
```

**Configuration:**
```
OKTA_DOMAIN                 - Okta tenant domain
PROVIDER_CLIENT_ID          - OAuth app client ID
PROVIDER_CLIENT_SECRET      - OAuth app secret
PROVIDER_REDIRECT_URI       - Callback URL
```

---

#### **Auth0Provider** (`auth/auth0.py`)

**Extends:** `OAuthCodeFlow`

**Use case:** Auth0 authentication platform

**Endpoints:**
```python
auth_url = f"https://{domain}/authorize"
token_url = f"https://{domain}/oauth/token"
logout_url = f"https://{domain}/v2/logout"
```

**Configuration:**
```
AUTH0_DOMAIN                - Auth0 tenant domain
PROVIDER_CLIENT_ID          - Application client ID
PROVIDER_CLIENT_SECRET      - Application secret
PROVIDER_REDIRECT_URI       - Callback URL
```

---

#### **LocalProvider** (`auth/local.py`)

**Use case:** File-based local authentication (no external IdP)

**Features:**
- ✅ ROPC (`authenticate()`) - Username/password verification
- ❌ Redirect OAuth - Not supported
- ✅ Roles - Loaded from JSON file

**User File Format** (`LOCAL_USERS` = `/etc/almond/users.conf`)

```json
{"username": "password_hash"}
{"username": {"password": "hash", "roles": ["admin", "operator"]}}
```

**Key Methods:**
```python
def authenticate(self, username, password):
    # Verify password hash using werkzeug.security.check_password_hash
    # Returns: {"username", "access_token", "roles", "provider": "local"}

def _load_users(self, path):
    # Parses JSON lines from users file
    # Each line: {"username": hash_or_dict_with_roles}
```

**Roles Handling:**
- If user entry is string: roles default to `["admin"]`
- If user entry is dict: roles from `value.get("roles", [])`

---

## 2. PROVIDER FACTORY & INITIALIZATION

### 2.1 ProviderFactory Pattern (`auth/factory.py`)

**Central point for instantiating providers:**

```python
class ProviderFactory:
    @staticmethod
    def create(provider_name, redirect_enabled, config):
        # provider_name: "keycloak", "entra", "okta", "auth0", "local"
        # redirect_enabled: bool - use OAuth redirect vs ROPC
        # config: AuthConfig object with all settings
```

**Provider Selection Logic:**

```python
if provider_name == "keycloak":
    if redirect_enabled:
        return KeycloakProvider(...)  # OAuth Code Flow
    else:
        return KeycloakROPC(...)      # ROPC

if provider_name == "entra":
    return EntraIDProvider(...)       # OAuth Code Flow only

if provider_name == "okta":
    return OktaProvider(...)          # OAuth Code Flow only

if provider_name == "auth0":
    return Auth0Provider(...)         # OAuth Code Flow only

if provider_name == "local":
    return LocalProvider(...)         # ROPC only

raise ValueError(f"Unknown provider: {provider_name}")
```

**Keycloak Dual Support:**
- Keycloak is the only provider supporting both ROPC and OAuth Code Flow
- Other external providers support OAuth Code Flow only
- Local provider supports ROPC only

---

### 2.2 Provider Instance Management (`auth/provider_instance.py`)

**Global provider singleton:**

```python
provider = None  # Module-level global

def set_provider(instance):
    global provider
    provider = instance

def get_provider():
    if provider is None:
        raise RuntimeError("Auth provider has not been initialized")
    return provider
```

**Used throughout the app:**
- `get_provider()` - Access current provider (must call `set_provider()` first)
- Single provider per application instance

---

## 3. ROLE EXTRACTION & STORAGE

### 3.1 Role Extraction from JWT (`login_handler.py`)

**Function:** `extract_roles_from_token(access_token: str) -> list`

**JWT Parsing (no signature verification):**
```python
decoded = jwt.get_unverified_claims(access_token)  # Decode without validation
```

**Role Extraction Priority (in order):**

1. **Keycloak Realm Roles** (top priority)
   ```python
   realm_roles = decoded.get("realm_access", {}).get("roles", [])
   ```

2. **Keycloak Client/Resource Roles**
   ```python
   resource_access = decoded.get("resource_access", {})
   for client, data in resource_access.items():
       client_roles = data.get("roles", [])
   ```

3. **Direct "roles" Claim**
   ```python
   if not roles:
       roles = decoded.get("roles", [])
   ```

4. **"groups" Claim**
   ```python
   if not roles:
       roles = decoded.get("groups", [])
   ```

5. **Custom Mapping** (fallback)
   ```python
   if not roles and username in CUSTOM_ROLE_MAPPING:
       roles = CUSTOM_ROLE_MAPPING[username]
   ```

6. **Default for Local**
   ```python
   if not roles and provider_name == "local":
       roles = ["admin"]
   ```

**JWT Structure Examples:**

**Keycloak Format:**
```json
{
  "realm_access": {
    "roles": ["realm-admin", "user"]
  },
  "resource_access": {
    "almond-api": {
      "roles": ["almond-exec", "viewer"]
    }
  }
}
```

**Entra/Okta/Auth0 Format:**
```json
{
  "roles": ["admin", "operator"],
  "groups": ["group1", "group2"]
}
```

---

### 3.2 Session Storage (`login_handler.py`)

**Function:** `create_session(user_dict, tokens_dict=None)`

**Flask Session Structure:**
```python
session = {
    "login": "true",
    "user": {
        "username": str,
        "provider": str,           # "keycloak", "entra", "okta", "auth0", "local"
        "source": str,             # "external" or "local"
        "roles": [str],            # List of role names
        "id_token": str,           # Optional: ID token from provider
        "access_token": str,       # OAuth access token
    },
    "tokens": {                    # Optional: for OAuth providers
        "access_token": str,
        "refresh_token": str,
        "id_token": str,
    }
}
```

**Role Access in Routes:**
```python
user = session.get("user", {})
user_roles = user.get("roles", [])
is_admin = "admin" in user_roles
```

---

### 3.3 Custom Role Mapping

**Setup (in howru.py or main app initialization):**
```python
from api.login_handler import set_custom_role_mapping

set_custom_role_mapping({
    "testuser": ["admin", "almond-exec"],
    "john@example.com": ["operator"],
    "jane@example.com": ["viewer"],
})
```

**Use Case:** When OAuth provider doesn't include roles in token or need role overrides

---

## 4. LOGIN FLOW: PROVIDER SELECTION

### 4.1 OAuth Redirect Flow (`howru.py`)

**Route: `/login`**

```
User clicks "Login" 
    ↓
GET /login
    ↓
1. Generate PKCE: verifier + challenge
2. Store in session["oauth_state"], session["pkce_verifier"]
3. Get authorization URL from get_provider().build_auth_redirect()
4. Redirect to provider's login URL
    ↓
User logs in at provider
    ↓
Provider redirects to /callback with code + state
    ↓
GET /callback?code=...&state=...
    ↓
1. Validate state matches session
2. Call get_provider().authenticate(code=code, verifier=verifier)
3. Exchange code for tokens
4. Call handle_oauth_login() to extract roles
5. Create Flask session
6. Redirect to /almond/admin
```

---

### 4.2 ROPC Flow (Basic Auth) - `admin_page.py`

**Form: POST /almond/admin `action_type=create_session`**

```
User submits username/password
    ↓
1. Provider = ProviderFactory.create(auth_provider_name, ...)
2. Call provider.authenticate(username=..., password=...)
    ↓
If provider supports ROPC:
    ↓
    Token data returned
    ↓
    Call handle_oauth_login() to extract roles
    ↓
    Create session
    ↓
If provider doesn't support ROPC:
    ↓
    Try fallback: verify_password() (local users file)
    ↓
    Create session with local auth
```

---

### 4.3 Provider Decision Logic (`admin_page.py`)

**Where provider is chosen:**

```python
# Line ~414-417
auth_provider_name = x[pos+1:].strip()  # From config: "api.authProvider"

if not auth_provider_name.lower() in ["keycloak", "entra", "oath0", "okta"]:
    auth_provider_name = "local"  # DEFAULT FALLBACK
```

**Configuration Source:**
- Read from config file: `api.authProvider=keycloak` (or entra, okta, auth0)
- Default: `"local"` if not specified or invalid

**Initialization:**
```python
# Line ~929
provider = ProviderFactory.create(auth_provider_name, enable_login_redirect, config=config)
set_provider(provider)
```

**Redirect vs ROPC Selection:**
```python
if not auth_init:
    provider = ProviderFactory.create(
        auth_provider_name,
        enable_login_redirect,  # Bool: use OAuth redirect?
        config=config
    )
```

- `enable_login_redirect=True` → Use OAuth Code Flow (if supported)
- `enable_login_redirect=False` → Use ROPC flow (if supported)
- Only Keycloak supports both modes

---

## 5. HARDCODED KEYCLOAK-SPECIFIC LOGIC

### 5.1 Role Extraction (Critical Issue)

**File:** `login_handler.py`, lines 55-74

```python
# These are KEYCLOAK-SPECIFIC token structures:
realm_roles = decoded.get("realm_access", {}).get("roles", [])
resource_access = decoded.get("resource_access", {})
for client, data in resource_access.items():
    client_roles = data.get("roles", [])
```

**Issue:** 
- ✅ Code tries to handle alternate claim names (`roles`, `groups`)
- ❌ **Primary check is Keycloak-specific** (`realm_access`, `resource_access`)
- ⚠️ If provider uses different claim structure, roles may not extract correctly

**Other Providers' Role Claims:**
- **Entra/Azure:** `roles`, `groups`
- **Okta:** `roles`, `groups`
- **Auth0:** `https://example.com/roles` (custom claim), `groups`

**Fallback handles this, but inefficiently:**
- First tries Keycloak structure
- Only then checks direct `roles` and `groups` claims

**Fix needed:** Reorder checks or use provider-specific extraction logic

---

### 5.2 Default Role Assignment

**File:** `login_handler.py`, line 234-237

```python
# Local provider fallback support
if not roles and provider_name == "local":
    roles = ["admin"]
    logger.info(f"[{provider_name}] Assigned default local role to '{username}'")
```

**Issue:**
- ✅ Only applies to `local` provider
- ❌ Hardcoded `"admin"` role for local users
- Local users file can override, but default is hardcoded

---

### 5.3 Scope Definition

**File:** Various providers

```python
# OAuthCodeFlow (base): "openid"
# KeycloakProvider: "openid offline_access"
# EntraIDProvider: "openid offline_access"
# OktaProvider: "openid offline_access"
# Auth0Provider: "openid offline_access"
```

**Issue:**
- Each provider hardcodes scope request
- No flexibility for adding custom scopes (e.g., for roles)
- Keycloak needs `offline_access` scope for refresh tokens

---

### 5.4 Logout URL Construction

**File:** Various providers

```python
# KeycloakProvider.logout_url():
f"{self.frontend_base}/realms/{self.realm}/protocol/openid-connect/logout"

# EntraIDProvider.logout_url():
f"https://login.microsoftonline.com/{self.tenant_id}/oauth2/v2.0/logout"

# OktaProvider.logout_url():
f"https://{self.domain}/oauth2/default/v1/logout"

# Auth0Provider.logout_url():
f"https://{self.domain}/v2/logout"
```

**Issue:**
- Each provider has different logout endpoint
- Keycloak uses `id_token_hint` parameter
- Others use `post_logout_redirect_uri`
- ⚠️ Not Keycloak-specific, but provider-aware logic required

---

### 5.5 Authentication Type Checking

**File:** `admin_page.py`, line 415

```python
if not auth_provider_name.lower() in ["keycloak", "entra", "oath0", "okta"]:
    auth_provider_name = "local"
```

**Issue:**
- ⚠️ Typo: `"oath0"` should be `"auth0"`
- If config has `auth0`, falls back to `local`
- **Blocks Auth0 provider** due to typo!

---

### 5.6 Configuration Defaults

**File:** `auth_config.py`, line 10

```python
AUTH_PROVIDER_NAME = "keycloak"  # HARDCODED DEFAULT
```

**Issue:**
- Default provider is always Keycloak
- Local authentication only if config explicitly sets it or invalid provider specified
- Favors Keycloak in design

---

### 5.7 Redirect vs ROPC Decision

**File:** `admin_page.py`, `factory.py`

```python
# Only Keycloak supports BOTH modes:
if provider_name == "keycloak":
    if redirect_enabled:
        return KeycloakProvider(...)      # OAuth
    else:
        return KeycloakROPC(...)          # ROPC

# All others: OAuth Code Flow only
if provider_name == "entra":
    return EntraIDProvider(...)           # OAuth only
```

**Issue:**
- ❌ Other providers can't do ROPC (password grant)
- ✅ Keycloak flexibility is a strength
- Not strictly "hardcoded Keycloak logic," but Keycloak gets special treatment

---

## 6. PROVIDER CAPABILITY MATRIX

| Provider | ROPC | OAuth Redirect | Userinfo | Refresh | Roles Support | Notes |
|----------|------|----------------|----------|---------|---------------|-------|
| **Keycloak ROPC** | ✅ | ❌ | ✅ | ❌ | Via token claims | Direct password auth |
| **Keycloak OAuth** | ❌ | ✅ | ❌ | ✅ | Via token claims | Needs realm_access/resource_access config |
| **Entra** | ❌ | ✅ | ❌ | ✅ | Via `roles` claim | getScope() typo - bug |
| **Okta** | ❌ | ✅ | ❌ | ✅ | Via `roles` claim | Standard OIDC |
| **Auth0** | ❌ | ✅ | ❌ | ✅ | Via custom claims | Works but typo blocks it |
| **Local** | ✅ | ❌ | ❌ | ❌ | Via JSON file | File-based users |

---

## 7. FLOW DIAGRAM: HOW LOGIN PROVIDER IS CHOSEN

```
Application Startup
    ↓
read_conf() in admin_page.py
    ↓
Look for "api.authProvider" in config file
    ↓
    ├─→ Found: Use value (keycloak, entra, okta, auth0, local)
    │       ↓
    │   Validate against whitelist ["keycloak", "entra", "oath0", "okta"]
    │       ↓
    │       ├─→ "auth0" found? Falls back to "local" (BUG: typo "oath0")
    │       │   ↓
    │       └─→ Valid? Use it
    │
    └─→ Not found: Default to "local"
            ↓
ProviderFactory.create(auth_provider_name, enable_login_redirect, config)
    ↓
    ├─→ "keycloak":
    │       ├─→ enable_login_redirect=True → KeycloakProvider (OAuth)
    │       └─→ enable_login_redirect=False → KeycloakROPC (ROPC)
    │
    ├─→ "entra" → EntraIDProvider (OAuth)
    ├─→ "okta" → OktaProvider (OAuth)
    ├─→ "auth0" → Auth0Provider (OAuth)
    └─→ "local" → LocalProvider (ROPC)
            ↓
set_provider(instance) - Store in global
            ↓
Use get_provider() throughout app
```

---

## 8. CRITICAL FINDINGS & RECOMMENDATIONS

### Issues Found

| Issue | Severity | File | Line | Fix |
|-------|----------|------|------|-----|
| Auth0 typo: `"oath0"` | 🔴 CRITICAL | admin_page.py | 415 | Change `"oath0"` → `"auth0"` |
| Keycloak-first role extraction | 🟡 MEDIUM | login_handler.py | 55-74 | Check provider type first, then role structure |
| Entra `getScope()` typo | 🟡 MEDIUM | entra.py | 29 | Change `getScope()` → `get_scope()` |
| Missing EntraID `get_scope()` | 🟡 MEDIUM | entra.py | - | Add missing method or inherit from base |
| No provider-specific role extraction | 🟡 MEDIUM | login_handler.py | - | Add provider-aware role extraction logic |
| Hardcoded default auth provider | 🟢 LOW | auth_config.py | 10 | Consider environment variable |

### Strengths

- ✅ Clean provider factory pattern
- ✅ Multiple OAuth providers supported
- ✅ Custom role mapping fallback
- ✅ PKCE implementation for OAuth
- ✅ Good logging for debugging
- ✅ Session management separated from authentication

### Recommendations

1. **Fix Auth0 typo** immediately (blocks Auth0 usage)
2. **Fix Entra scope method** (missing method call)
3. **Reorganize role extraction** to check provider first
4. **Add provider-specific role mappers** (avoid Keycloak-centric checks)
5. **Document role claim names** for each provider
6. **Add unit tests** for role extraction per provider
7. **Make default provider configurable** via environment variable

---

## 9. CONFIGURATION REFERENCE

### Full Configuration Variables

**Keycloak ROPC:**
```
KEYCLOAK_TOKEN_URL
KEYCLOAK_USERINFO_URL
KEYCLOAK_CLIENT_ID
KEYCLOAK_CLIENT_SECRET
```

**Keycloak OAuth:**
```
PROVIDER_FRONTEND_BASE_URL
PROVIDER_BACKEND_BASE_URL
PROVIDER_REALM
PROVIDER_CLIENT_ID
PROVIDER_CLIENT_SECRET
PROVIDER_REDIRECT_URI
```

**Entra:**
```
PROVIDER_TENANT_ID
PROVIDER_CLIENT_ID
PROVIDER_CLIENT_SECRET
PROVIDER_REDIRECT_URI
```

**Okta:**
```
OKTA_DOMAIN
PROVIDER_CLIENT_ID
PROVIDER_CLIENT_SECRET
PROVIDER_REDIRECT_URI
```

**Auth0:**
```
AUTH0_DOMAIN
PROVIDER_CLIENT_ID
PROVIDER_CLIENT_SECRET
PROVIDER_REDIRECT_URI
```

**Local:**
```
LOCAL_USERS (default: /etc/almond/users.conf)
```

**Enable OAuth Redirect:**
```
api.enableLoginRedirect (config file)
enable_login_redirect (variable in admin_page.py)
```

**Select Provider:**
```
api.authProvider (config file: keycloak, entra, okta, auth0, local)
auth_provider_name (variable in admin_page.py)
```

---

## 10. TESTING EACH PROVIDER

### Keycloak ROPC
```
Provider: keycloak
enable_login_redirect: false
Test: POST to /almond/admin with username/password
Expected: Roles from Keycloak token
```

### Keycloak OAuth
```
Provider: keycloak
enable_login_redirect: true
Test: Click login, redirect to Keycloak, authorize, callback
Expected: Roles from Keycloak token (realm_access.roles + resource_access)
```

### Entra
```
Provider: entra
enable_login_redirect: true
Test: Click login, redirect to Azure, authorize, callback
Expected: Roles from 'roles' or 'groups' claim
```

### Okta
```
Provider: okta
enable_login_redirect: true
Test: Click login, redirect to Okta, authorize, callback
Expected: Roles from 'roles' claim
```

### Auth0 (Currently broken due to typo)
```
Provider: auth0 (currently falls back to local due to typo)
enable_login_redirect: true
Test: Click login, redirect to Auth0, authorize, callback
Expected: Roles from custom claims or 'roles' claim
```

### Local
```
Provider: local
Test: POST to /almond/admin with username/password from /etc/almond/users.conf
Expected: Roles from users.conf file
```

---

## Document Version
- **Version:** 1.0
- **Date:** April 20, 2026
- **Scope:** Complete authentication system analysis
- **Status:** All providers documented, issues identified
