# Keycloak Role Configuration

## Current Status

Your Keycloak instance is working for authentication, but **roles are not being included in the access token**. You have an existing `almond-api` client with the `almond-exec` role already configured.

## Your Setup

You have:
- ✅ `almondadmin` client (for authentication/login)
- ✅ `almond-api` client (with `almond-exec` role)
- ✅ User `testuser` assigned to `almond-api` client with `almond-exec` role

## Why Roles Aren't in the Token

The `almondadmin` client is not requesting or including roles. Your current token only contains:
- `sub` (user ID)
- `azp` (authorized party - client ID: almondadmin)
- `scope` (openid, offline_access)
- Basic claims (iss, iat, exp, etc.)

**Missing:**
- `realm_access.roles` (realm-level roles)
- `resource_access.almond-api.roles` (client-level roles from almond-api)

## Solution: Use Protocol Mappers

Since your Keycloak version has different tabs, here's how to find Protocol Mappers:

### Locate Protocol Mappers in Keycloak 24

In Keycloak 24, the client page no longer always shows a separate "Mappers" tab the way older versions did.
Instead, role mappers are usually managed through **Client Scopes** or the client "Scope" configuration.

1. **Open Keycloak Admin Console**
   - URL: `http://localhost:8089/admin`

2. **Navigate to:** Realms → almondmonitor → Clients → almondadmin

3. **If you do not see a Mappers tab:**
   - Look for a **Client Scopes** section/tab inside the almondadmin client
   - Or use the global **Client Scopes** menu in the realm sidebar
   - In Keycloak 24, you often create a scope and attach it to the client rather than adding mappers directly on the client

### Use Client Scopes for Keycloak 24

1. **Create a new client scope**
   - Realms → almondmonitor → Client Scopes
   - Click "Create"
   - Name: `roles`
   - Protocol: `openid-connect`
   - Click "Save"

2. **Add mappers to the scope**
   - Open the new `roles` scope
   - Find the "Mappers" section inside that scope
   - Add one mapper for realm roles and one for client roles

3. **Attach the scope to the almondadmin client**
   - Open Clients → almondadmin → Client Scopes
   - Add the new `roles` client scope as optional or default

4. **Assign roles to users**
   - Open Users → select user → Role Mappings
   - Add the appropriate realm or client roles

### If you already have `almond-api` configured

Because `almond-api` already has the `almond-exec` role, you can also:
- check `almond-api` for existing client scopes/mappers
- attach those scopes to `almondadmin`
- or use `almond-api` as the authorization client if it is the one carrying roles

### Summary for Keycloak 24

- `Mappers` may not appear as a top-level client tab
- Use the realm-level **Client Scopes** workflow instead
- Create a scope, add role mappers there, attach it to `almondadmin`
- Make sure `Add to access token` is enabled for the mappers

### Direct Alternative: Use almond-api Client

Since `almond-api` already has the `almond-exec` role configured, you could:

1. **Update configuration to use almond-api instead:**
   - In your auth configuration, redirect to use `almond-api` client
   - Or add mappers to `almond-api` to expose its roles

2. **Check if almond-api has mappers:**
   - Go to Realms → almondmonitor → Clients → almond-api
   - Look for tabs or options for mappers/roles configuration

## Your Keycloak Version

Your tab layout (Settings, Keys, Credentials, Roles, Client Sessions, Advanced) suggests you're using **Keycloak 19+** or a custom build.

**To find Protocol Mappers in your version:**

1. Go to almondadmin Client
2. Check each tab in this order:
   - **Advanced** - might be under Advanced settings
   - **Credentials** - might be combined here
   - Try scrolling down in **Settings** - might be below

3. **If still not visible:**
   - Look for a "Configure" or "Options" icon
   - Check for a sidebar menu in the client view
   - Try clicking on client name to see if it expands options

## Workaround: Use Custom Role Mapping

While you locate the Mappers feature, use the custom role mapping approach:

```python
# In admin_page.py or howru.py initialization:
from api.login_handler import set_custom_role_mapping

set_custom_role_mapping({
    "testuser": ["admin", "almond-exec"],  # Your test user with roles
    # Add other users as needed
})
```

This immediately gives your users roles without needing Keycloak token configuration.

## Get Token Structure Info

To debug your Keycloak setup, log in and check what's actually in the token:

Look for logs like:
```
DEBUG: [keycloak] Access token keys: dict_keys([...])
DEBUG: [keycloak] Access token claims: {...}
```

Share what you see in the access token claims, especially if you see:
- `realm_access`
- `resource_access`
- `roles`
- Any other role-related fields

This will help identify the right configuration path for your version.

## Related Files

- `login_handler.py` - Handles role extraction
- `admin_page.py` - Uses roles for authorization
- `QUICK_ROLE_INTEGRATION.md` - Quick setup with custom role mapping
- `ROLE_AUTHORIZATION_EXAMPLES.md` - How to use roles

---

**Next: Please check your Keycloak version and tab structure, then we can locate Protocol Mappers or find the best alternative for your setup.**

