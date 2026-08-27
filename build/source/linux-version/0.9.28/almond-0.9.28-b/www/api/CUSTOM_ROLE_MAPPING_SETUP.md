# Using Custom Role Mapping (Keycloak Setup)

## What Was Just Done

Added custom role mapping to `howru.py` so your users get roles without needing Keycloak token configuration:

```python
set_custom_role_mapping({
    "testuser": ["admin", "almond-exec"],
    # Add other users as needed
})
```

## Verify It's Working

1. **Restart the application**
   - For changes to take effect

2. **Log in with testuser**
   - Check the logs for:
   ```
   INFO: Assigned custom roles for user 'testuser': ['admin', 'almond-exec']
   INFO: User 'testuser' logged in via keycloak. Roles: ['admin', 'almond-exec']
   ```

3. **Verify testuser has `almond-exec` role:**
   - They should be able to execute admin functions
   - Authorization checks will recognize them as admin

## Add More Users

Edit `howru.py` and add more users to the mapping:

```python
set_custom_role_mapping({
    "testuser": ["admin", "almond-exec"],
    "john.doe@example.com": ["operator"],
    "jane.smith@example.com": ["viewer"],
    "another_user": ["admin"],
    # Format: "username_or_email": ["role1", "role2", ...]
})
```

**Finding User Identifiers:**

To find the correct username/email to use:

1. Go to Keycloak Admin Console
2. Realms → almondmonitor → Users
3. Click on the user
4. Note their:
   - **Username** field (usually in account details)
   - **Email** field (if they have one)
   - These are what you use in the mapping

**Which identifier to use?**

The mapping uses the username that appears in the JWT token's `sub` claim. From your logs:
```
User 8d6a4669-9c19-4e41-aa7b-486bf571f833 logged in via keycloak
```

This long ID is the `sub` claim. The username displayed ("testuser") is what we match.

If "testuser" doesn't work, try:
- The user's email
- The exact case-sensitive username from Keycloak
- Check logs to see what username is being extracted

## Available Roles

You can assign any role names. Common choices:

- `admin` - Full administrative access
- `operator` - Can execute operations
- `viewer` - Read-only access
- `almond-exec` - Can execute almond commands
- Custom roles as needed

## Role Hierarchy

Suggested role structure:
```python
set_custom_role_mapping({
    # Admins - full access
    "admin_user": ["admin"],
    
    # Operators - can execute but not configure
    "operator_user": ["operator"],
    
    # Viewers - read-only
    "viewer_user": ["viewer"],
    
    # Specific permissions
    "executor_user": ["almond-exec"],
})
```

## Using Roles in Code

Once users have roles, protect actions:

```python
from api.login_handler import check_authorization, get_current_user

# In admin_page.py action handlers:

if action_type == 'restart_almond':
    check_authorization("admin")  # Only admins can restart
    user = get_current_user()
    logger.info(f"Restart by {user['username']} with roles {user['roles']}")
    # ... restart code ...

elif action_type == 'execute_plugin':
    check_authorization(["admin", "almond-exec"])  # Admins or almond-exec users
    # ... execution code ...

elif action_type == 'view_logs':
    check_authorization(["admin", "operator", "viewer"])  # Everyone can view
    # ... log viewing code ...
```

## Logging

All role assignments are logged:
```
DEBUG: [keycloak] Extracted roles: []
INFO: Assigned custom roles for user 'testuser': ['admin', 'almond-exec']
DEBUG: [keycloak] ID token claims extracted
```

Check `/var/log/almond/howru.log` to verify users get their roles.

## Load from Configuration File

For production, you can load role mapping from a config file:

```python
# In howru.py
import json
from api.login_handler import set_custom_role_mapping

try:
    with open('/etc/almond/user_roles.json', 'r') as f:
        user_roles = json.load(f)
    set_custom_role_mapping(user_roles)
except FileNotFoundError:
    # Use hardcoded defaults if file doesn't exist
    set_custom_role_mapping({
        "testuser": ["admin", "almond-exec"],
    })
except Exception as e:
    logger.error(f"Failed to load user roles: {e}")
```

Then create `/etc/almond/user_roles.json`:
```json
{
  "testuser": ["admin", "almond-exec"],
  "john.doe@example.com": ["operator"],
  "jane.smith@example.com": ["viewer"],
  "bob.wilson@example.com": ["admin"]
}
```

Benefit: Update roles without restarting the app (restart needed though for this version).

## Next Steps: Proper Keycloak Configuration

This custom mapping is a temporary solution. For production:

1. **Find Protocol Mappers in your Keycloak version:**
   - Check "Advanced" tab in client settings
   - Or look for role mapper options
   - Contact Keycloak documentation for your version

2. **Once configured:**
   - Roles will be in the token automatically
   - Remove or simplify custom mapping
   - Roles become dynamic from Keycloak

3. **In the meantime:**
   - Use custom mapping (this approach)
   - Get role-based authorization working
   - Plan Keycloak configuration for later

## Troubleshooting

**Users not getting custom roles?**
- Check the exact username they use to log in
- Verify the mapping has the correct username
- Check logs for what username is extracted
- Try user's email instead

**Roles not working in authorization checks?**
- Verify user has the role in the mapping
- Check logs show role was assigned
- Make sure you're using @require_role or check_authorization correctly
- See ROLE_AUTHORIZATION_EXAMPLES.md

**Want to remove custom mapping later?**
- Just remove or comment out the `set_custom_role_mapping()` call
- Roles will fall back to token (if Keycloak configured) or admin default

## Related Files

- `login_handler.py` - Role mapping implementation
- `ROLE_AUTHORIZATION_EXAMPLES.md` - How to use roles in code
- `QUICK_ROLE_INTEGRATION.md` - Alternative integration methods
- `KEYCLOAK_ROLE_CONFIGURATION.md` - Proper Keycloak setup
