# Quick Integration Guide - Role Assignment

## Current Situation

Your Keycloak login is working, but roles aren't in the token. Users get a default `admin` role.

## Option 1: Use Custom Role Mapping (Quick Fix)

If you want to assign different roles to different users **without configuring Keycloak**:

### In `admin_page.py` or your initialization code:

```python
from api.login_handler import set_custom_role_mapping

# Define your user-to-roles mapping
user_roles = {
    "john.doe@example.com": ["admin"],
    "jane.smith@example.com": ["operator", "viewer"],
    "bob.wilson@example.com": ["viewer"],
}

# Set the mapping
set_custom_role_mapping(user_roles)
```

Now when users log in:
- John will have `admin` role
- Jane will have `operator` and `viewer` roles  
- Bob will have `viewer` role
- Other users get default `admin` role

### Where to Add This

Add it to `howru.py` after creating the Flask app:

```python
# howru.py
app = flask.Flask(__name__)
app.config["DEBUG"] = True

# ... other config ...

# Add this after basic setup:
from api.login_handler import set_custom_role_mapping

# Configure custom roles for users without Keycloak mapper
set_custom_role_mapping({
    "admin_user": ["admin"],
    "operator_user": ["operator"],
})

# ... rest of code ...
```

## Option 2: Proper Keycloak Configuration (Recommended)

For production, configure Keycloak to include roles in the token:

See [KEYCLOAK_ROLE_CONFIGURATION.md](KEYCLOAK_ROLE_CONFIGURATION.md)

Once Keycloak is configured:
1. Roles will be extracted from the token automatically
2. Custom mapping becomes optional (roles from token take priority)
3. No code changes needed

## Option 3: Load Roles from File

If you have a configuration file with role mappings:

```python
import json

def load_role_mapping(filepath):
    """Load role mapping from JSON file"""
    try:
        with open(filepath, 'r') as f:
            mapping = json.load(f)
        from api.login_handler import set_custom_role_mapping
        set_custom_role_mapping(mapping)
        logger.info(f"Loaded role mapping for {len(mapping)} users")
    except Exception as e:
        logger.error(f"Failed to load role mapping: {e}")

# In initialization:
load_role_mapping('/etc/almond/user_roles.json')
```

Example `/etc/almond/user_roles.json`:
```json
{
  "admin@example.com": ["admin"],
  "operator@example.com": ["operator"],
  "viewer@example.com": ["viewer"]
}
```

## Verification

After setting up role mapping:

1. **Check the logs:**
```
INFO: Assigned custom roles for user 'john.doe': ['admin']
DEBUG: [keycloak] Extracted roles: ['admin']
INFO: User 'john.doe' logged in via keycloak. Roles: ['admin']
```

2. **Verify in admin page:**
   - Log in as different users
   - Check if actions are restricted appropriately
   - Use `@require_admin` decorator to test authorization

## Test Authorization

Once roles are assigned:

```python
from api.login_handler import require_admin, get_user_roles

@app.route('/admin/restart')
@require_admin  # Only users with 'admin' role
def restart_service():
    user = get_current_user()
    logger.info(f"Restart requested by {user['username']} with roles {user['roles']}")
    # ... restart code ...
```

## Logging

All role assignments are logged:
- `DEBUG: [keycloak] Extracted roles: [...]`
- `INFO: Assigned custom roles for user '...': [...]`
- `INFO: Assigned default admin role to '...'`

Check `/var/log/almond/howru.log` to see what roles each user gets.

## Hierarchy

Role assignment priority:
1. **First:** Roles from token (if Keycloak configured)
2. **Second:** Custom mapping (if set)
3. **Default:** `admin` role (everyone gets this as fallback)

---

**Next Steps:**
- ✅ Login works with Keycloak
- ✅ Users have roles (either custom or default)
- 🔄 Add role-based authorization to actions
- 🔄 Configure Keycloak properly (optional but recommended)

See `ROLE_AUTHORIZATION_EXAMPLES.md` for how to use roles in your code.
