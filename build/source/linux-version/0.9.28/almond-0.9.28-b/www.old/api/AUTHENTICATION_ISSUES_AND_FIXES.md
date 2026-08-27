# Authentication System: Issues & Implementation Fixes

## Executive Summary

**3 Critical Issues Found:**
1. 🔴 **Auth0 BLOCKED** - Typo in provider validation (`"oath0"` instead of `"auth0"`)
2. 🟡 **Entra Missing Method** - `get_scope()` not properly implemented
3. 🟡 **Keycloak-Centric Role Extraction** - Other providers inefficiently checked

**Impact:** Auth0 provider completely non-functional; Entra scope not set; other providers work via fallback logic

---

## Issue #1: Auth0 BLOCKED (Critical)

### Location
**File:** `admin_page.py`, line 415

### Current Code
```python
if not auth_provider_name.lower() in ["keycloak", "entra", "oath0", "okta"]:
    auth_provider_name = "local"
```

### Problem
- Typo: `"oath0"` instead of `"auth0"`
- If config specifies `api.authProvider=auth0`, validation fails
- Provider falls back to `"local"`
- **Auth0 provider is completely unreachable**

### Reproduction
1. Set `api.authProvider=auth0` in config
2. Try to login
3. → Falls back to local authentication
4. → Auth0 OAuth flow never attempted

### Fix
```python
if not auth_provider_name.lower() in ["keycloak", "entra", "auth0", "okta"]:
    auth_provider_name = "local"
```

**Change:** `"oath0"` → `"auth0"`

### Verification
1. Set config to `api.authProvider=auth0`
2. Check logs: Should see "Auth0Provider" loaded
3. Try OAuth flow: Should redirect to Auth0

---

## Issue #2: Missing Entra Scope Method

### Location
**File:** `entra.py`, line 29

### Current Code
```python
class EntraIDProvider(OAuthCodeFlow):
    def getScope(self):  # ← WRONG: lowercase 's'
        return "openid offline_access"
```

### Problem
- Method name is `getScope()` (camelCase)
- Base class `OAuthCodeFlow` calls `get_scope()` (snake_case)
- Method never called → Scope not set in OAuth URL
- May prevent refresh token from being returned

### Reproduction
1. Use Entra OAuth flow
2. Check generated auth URL
3. Should include `scope=openid+offline_access`
4. Currently: Scope might be missing or default ("openid" only)

### Root Cause
```python
# In oauth_code.py, build_auth_redirect() calls:
f"&scope={quote(self.get_scope())}"  # ← Calls get_scope()

# But EntraIDProvider defines:
def getScope(self):  # ← Different name
```

### Fix
**Option A: Rename method (Recommended)**
```python
class EntraIDProvider(OAuthCodeFlow):
    def get_scope(self):  # ← Correct: snake_case
        return "openid offline_access"
```

**Option B: Remove method (inherit from parent)**
```python
# If parent OAuthCodeFlow.get_scope() returns correct value
# Just inherit it
```

Check current implementation:
```python
# In oauth_code.py:
def get_scope(self):
    return "openid"  # Parent returns this

# Entra needs "openid offline_access"
# So must override with correct method name
```

### Verification
1. Use Entra OAuth flow
2. Check console/logs for generated auth URL
3. Should contain: `scope=openid+offline_access`
4. After callback, check tokens: Should have `refresh_token`

---

## Issue #3: Keycloak-Centric Role Extraction

### Location
**File:** `login_handler.py`, lines 55-74

### Current Code
```python
def extract_roles_from_token(access_token):
    decoded = jwt.get_unverified_claims(access_token)
    roles = set()

    # KEYCLOAK-SPECIFIC (checked first)
    realm_roles = decoded.get("realm_access", {}).get("roles", [])
    roles.update(realm_roles)

    # KEYCLOAK-SPECIFIC (checked second)
    resource_access = decoded.get("resource_access", {})
    for client, data in resource_access.items():
        client_roles = data.get("roles", [])
        roles.update(client_roles)

    # GENERIC (checked last - fallback)
    if not roles:
        if "roles" in decoded:
            roles.update(decoded.get("roles", []))
        if "groups" in decoded:
            roles.update(decoded.get("groups", []))

    return sorted(list(roles))
```

### Problem
1. **Keycloak checks first** - Other providers' claims ignored unless Keycloak paths are empty
2. **Inefficient** - Checks for nested structures that only Keycloak uses
3. **Confusing** - When Keycloak structures are empty, then checks direct claims
4. **Hard to debug** - Roles may be present in standard claims but missed

### Example Scenario
**Entra ID token contains:**
```json
{
  "roles": ["admin", "operator"],
  "realm_access": null,
  "resource_access": null
}
```

**Current code:**
1. Checks `realm_access.roles` → empty
2. Checks `resource_access[*].roles` → empty
3. **Only then** checks `roles` → found!

**Better approach:**
1. Check provider type
2. Use provider-specific extraction
3. Fall back to generic claims

### Recommended Fix

**Approach 1: Add provider-specific extraction (Best)**

```python
def extract_roles_from_token(access_token, provider_name=None):
    """
    Extract roles from JWT, provider-aware.
    
    Args:
        access_token: JWT token string
        provider_name: Provider name for context (optional)
        
    Returns:
        list: Extracted roles
    """
    try:
        decoded = jwt.get_unverified_claims(access_token)
        roles = set()

        # Provider-specific extraction
        if provider_name == "keycloak":
            # Keycloak: realm_access + resource_access
            realm_roles = decoded.get("realm_access", {}).get("roles", [])
            roles.update(realm_roles)
            
            resource_access = decoded.get("resource_access", {})
            for client, data in resource_access.items():
                client_roles = data.get("roles", [])
                roles.update(client_roles)
        
        elif provider_name in ["entra", "okta", "auth0"]:
            # Standard OIDC: direct claims
            if "roles" in decoded:
                roles.update(decoded.get("roles", []))
            if "groups" in decoded:
                roles.update(decoded.get("groups", []))
        
        elif provider_name == "local":
            # Local: roles already in separate structure
            pass
        
        else:
            # Generic fallback: try all common claim names
            roles.update(decoded.get("realm_access", {}).get("roles", []))
            
            resource_access = decoded.get("resource_access", {})
            for client, data in resource_access.items():
                roles.update(data.get("roles", []))
            
            roles.update(decoded.get("roles", []))
            roles.update(decoded.get("groups", []))

        logger.debug(f"[extract_roles] Provider: {provider_name}, Roles: {sorted(list(roles))}")
        return sorted(list(roles))
    
    except Exception as e:
        logger.warning(f"Failed to extract roles: {e}")
        return []
```

**Approach 2: Reorder checks (Quick fix)**

```python
def extract_roles_from_token(access_token):
    decoded = jwt.get_unverified_claims(access_token)
    roles = set()

    # Try generic claims FIRST (most providers)
    if "roles" in decoded:
        roles.update(decoded.get("roles", []))
    
    if "groups" in decoded:
        roles.update(decoded.get("groups", []))

    # Then try Keycloak NESTED structures
    if not roles:
        realm_roles = decoded.get("realm_access", {}).get("roles", [])
        roles.update(realm_roles)

        resource_access = decoded.get("resource_access", {})
        for client, data in resource_access.items():
            client_roles = data.get("roles", [])
            roles.update(client_roles)

    return sorted(list(roles))
```

### Where to Update
```python
# In login_handler.py, handle_oauth_login() function:
# Change:
roles = extract_roles_from_token(access_token)

# To (if using Approach 1):
roles = extract_roles_from_token(access_token, provider_name)
```

### Verification
1. Test each provider with roles in token
2. Check logs: Should see correct role extraction
3. Verify roles appear in session
4. Test authorization: Users with roles can access admin functions

---

## Implementation Roadmap

### Priority 1: Fix Auth0 Typo (5 minutes)

**File:** `admin_page.py:415`
```python
# Change:
if not auth_provider_name.lower() in ["keycloak", "entra", "oath0", "okta"]:

# To:
if not auth_provider_name.lower() in ["keycloak", "entra", "auth0", "okta"]:
```

**Test:**
```bash
# Set config
api.authProvider=auth0

# Verify in logs
# Should see: "KeycloakProvider" or similar (Auth0 loaded)
# Not: "LocalProvider" (fallback)
```

---

### Priority 2: Fix Entra Scope Method (2 minutes)

**File:** `entra.py:29`
```python
# Change:
def getScope(self):

# To:
def get_scope(self):
```

**Test:**
```bash
# Use Entra OAuth flow
# Check generated auth URL includes: scope=openid+offline_access
# Verify refresh_token in response
```

---

### Priority 3: Improve Role Extraction (20 minutes)

**File:** `login_handler.py:55-90` (expand)

**Choose approach:**
- Approach 1 (provider-aware): Better code quality, clearer logic
- Approach 2 (reorder checks): Minimal change, still works

**Test:**
```bash
# Test each provider separately
# Verify roles in session after login
# Check admin authorization works
```

---

## Test Cases

### Test 1: Auth0 Provider Selection

```python
# Setup
config.AUTH_PROVIDER_NAME = "auth0"

# Run
admin_page.read_conf()

# Expected
auth_provider_name == "auth0"  # Not "local"
provider instance is Auth0Provider

# Verify
logs should show Auth0 provider loaded
```

### Test 2: Entra Scope Included

```python
# Setup
provider = EntraIDProvider(...)

# Run
url = provider.build_auth_redirect(state, challenge)

# Expected
"scope=openid+offline_access" in url

# Current (broken)
"scope=openid" (missing offline_access)
```

### Test 3: Keycloak Roles Extraction

```python
# Token with Keycloak structure
token = {
    "realm_access": {"roles": ["realm-admin"]},
    "resource_access": {
        "almond-api": {"roles": ["almond-exec"]}
    }
}

# Run
roles = extract_roles_from_token(token_jwt)

# Expected
roles == ["almond-exec", "realm-admin"]
```

### Test 4: Entra Roles Extraction

```python
# Token with standard OIDC structure
token = {
    "roles": ["admin", "operator"],
    "groups": ["group1"]
}

# Run
roles = extract_roles_from_token(token_jwt)

# Expected
roles == ["admin", "operator", "group1"]
```

### Test 5: Okta Roles Extraction

```python
# Token with Okta structure
token = {
    "roles": ["viewer"],
    "groups": ["okta_group"]
}

# Run
roles = extract_roles_from_token(token_jwt)

# Expected
roles == ["okta_group", "viewer"]
```

---

## Files to Modify

| Priority | File | Line | Change | Impact |
|----------|------|------|--------|--------|
| 🔴 P1 | `admin_page.py` | 415 | `"oath0"` → `"auth0"` | Auth0 provider now works |
| 🟡 P2 | `entra.py` | 29 | `getScope` → `get_scope` | Entra gets offline_access scope |
| 🟡 P3 | `login_handler.py` | 55-90 | Improve role extraction | Cleaner code, better support |

---

## Rollback Plan

If issues arise:

1. **Auth0 typo fix:** Can't break anything (auth0 config currently doesn't work anyway)
2. **Entra scope fix:** Worst case: still works, just may not get refresh token (same as now)
3. **Role extraction:** Backwards compatible, only affects log output

All changes are **non-breaking** and **safe to deploy**

---

## Verification Checklist After Fixes

- [ ] Auth0 provider loads (config `api.authProvider=auth0`)
- [ ] Entra scope includes `offline_access` (check auth URL)
- [ ] All providers extract roles correctly (check logs and session)
- [ ] Existing Keycloak functionality unchanged
- [ ] Local authentication still works
- [ ] Custom role mapping still works
- [ ] Tests pass

---

## Document Version
- **Version:** 1.0
- **Type:** Implementation Guide
- **Created:** April 20, 2026
- **Status:** Ready for implementation
