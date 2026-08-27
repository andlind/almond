# Login Handler Refactoring - Documentation

## Overview

The login system has been refactored to be **cleaner, more maintainable, and role-aware**. The code is now separated into:

1. **Authentication** - User login and token exchange
2. **Authorization** - Role-based access control
3. **Session Management** - User session lifecycle

---

## New Architecture

### 1. `login_handler.py` - Central Module

A new module that provides all login and authorization functionality:

```
/api/login_handler.py
├── Role Management Functions
├── Authentication Functions  
├── Session Functions
└── Authorization Decorators
```

**File Location:** `/Users/alio/git/private/almond/build/source/linux-version/0.9.28/almond-0.9.28/www/api/login_handler.py`

---

## Key Improvements

### Before (Messy)
```python
# Old approach - scattered, hard to follow
def authenticate(username, password):
    global enable_oath
    if enable_oath:
        oath = external_auth(username, password)
        if oath:
            return {"source": "external", "username": username, "token": oath}
    if (verify_password(username, password)):
        return {"source": "local", "username": username}

def extract_roles(access_token):
    # ... token parsing code
    return list(roles)

# In view function:
if token_data:
    access_token = token_data.get("access_token")
    userinfo = provider.get_userinfo(access_token) or {}
    # ... lots of if/else logic ...
    session["user"] = {
        "username": username_value,
        "provider": provider.name
    }
```

### After (Clean)
```python
# New approach - clear and centralized
from api.login_handler import (
    handle_oauth_login,
    create_session,
    get_user_roles,
    is_admin,
)

# In view function:
user_session = handle_oauth_login(provider, token_data, auth_provider_name)
if user_session:
    create_session(user_session)
    logger.info(f"Roles: {user_session.get('roles', [])}")
```

---

## User Session Structure (NEW)

Users now have roles included in their session:

```python
session["user"] = {
    "username": "john.doe",
    "provider": "keycloak",  # or "local", "entra", "okta", etc.
    "source": "external",    # or "local"
    "roles": ["admin", "operator", "viewer"],  # NEW - extracted from token
    "id_token": "...",
    "access_token": "..."
}
```

---

## API Reference

### Role Management

#### `extract_roles_from_token(access_token)`
Extract roles from JWT token
```python
from api.login_handler import extract_roles_from_token

roles = extract_roles_from_token(access_token)
# Returns: ["admin", "operator", "viewer"]
```

#### `user_has_role(required_role)`
Check if current user has specific role
```python
from api.login_handler import user_has_role

if user_has_role("admin"):
    # Allow admin operation
```

#### `user_has_any_role(required_roles)`
Check if user has any of the roles
```python
from api.login_handler import user_has_any_role

if user_has_any_role(["admin", "operator"]):
    # Allow admin or operator action
```

#### `get_user_roles()`
Get all roles for current user
```python
from api.login_handler import get_user_roles

roles = get_user_roles()
# Returns: ["admin", "operator"]
```

#### `is_admin(user_dict)`
Check if user is admin
```python
from api.login_handler import is_admin

user = session.get("user")
if is_admin(user):
    # Admin-only code
```

### Authentication

#### `handle_oauth_login(provider, token_data, provider_name)`
Process OAuth login and extract roles
```python
from api.login_handler import handle_oauth_login

user_session = handle_oauth_login(provider, token_data, "keycloak")
# Returns: {
#     "username": "john",
#     "provider": "keycloak",
#     "roles": ["admin"],
#     ...
# }
```

#### `handle_local_login(username)`
Process local user login
```python
from api.login_handler import handle_local_login

user_session = handle_local_login("admin")
# Returns: {
#     "username": "admin",
#     "provider": "local",
#     "roles": ["admin"],  # Local users get admin by default
# }
```

#### `create_session(user_dict, tokens_dict=None)`
Create Flask session for user
```python
from api.login_handler import create_session

create_session(user_session, tokens)
# Sets session["login"] = "true"
# Sets session["user"] = user_session
# Sets session["tokens"] = tokens (optional)
```

#### `clear_session()`
Logout user and clear session
```python
from api.login_handler import clear_session

clear_session()  # Removes all session data
```

#### `is_logged_in()`
Check if user has valid session
```python
from api.login_handler import is_logged_in

if is_logged_in():
    # User is authenticated
```

#### `get_current_user()`
Get current logged-in user
```python
from api.login_handler import get_current_user

user = get_current_user()
# Returns user dict or None
```

### Authorization Decorators

#### `@require_login`
Require user to be logged in
```python
from api.login_handler import require_login

@app.route('/admin')
@require_login
def admin_page():
    return render_template('admin.html')
    # Returns 403 if not logged in
```

#### `@require_role(role_name)`
Require specific role
```python
from api.login_handler import require_role

@app.route('/admin/restart')
@require_role('admin')
def restart_service():
    # Code here only runs if user has 'admin' role
    # Returns 403 otherwise
```

#### `@require_any_role(*roles)`
Require any of multiple roles
```python
from api.login_handler import require_any_role

@app.route('/operations')
@require_any_role('admin', 'operator')
def operations_page():
    # Allows users with 'admin' OR 'operator' role
    # Returns 403 otherwise
```

#### `@require_admin`
Require admin role
```python
from api.login_handler import require_admin

@app.route('/admin/config')
@require_admin
def admin_config():
    # Admin-only route
    # Returns 403 if not admin
```

#### `check_authorization(required_roles)`
Inline authorization check
```python
from api.login_handler import check_authorization

if action_type == "restart_almond":
    check_authorization("admin")  # Raises 403 if not authorized
    # Proceed with restart
```

---

## Usage Examples

### Example 1: Protect Admin Route

**Old way (mixed concerns):**
```python
@app.route('/admin/restart')
def restart():
    if 'login' not in session:
        return render_template('login.html'), 401
    # ... restart logic ...
```

**New way (clear intent):**
```python
from api.login_handler import require_admin

@app.route('/admin/restart')
@require_admin
def restart():
    # Code here is only reached if user is admin
    # Returns 403 automatically if not
    # ... restart logic ...
```

### Example 2: Check Roles in Action

**Old way:**
```python
if action_type == "execute_plugin":
    # No role checking - anyone can execute!
    # ... plugin execution ...
```

**New way:**
```python
from api.login_handler import check_authorization, get_current_user

if action_type == "execute_plugin":
    check_authorization(["admin", "operator"])  # Only admins or operators
    user = get_current_user()
    logger.info(f"User {user['username']} executing plugin. Roles: {user['roles']}")
    # ... plugin execution ...
```

### Example 3: Role-Based UI/Logic

```python
from api.login_handler import get_user_roles, user_has_role, get_current_user

def render_admin_page():
    user = get_current_user()
    roles = get_user_roles()
    is_super_admin = user_has_role("super_admin")
    
    # Customize template based on roles
    return render_template('admin.html', 
        user=user,
        roles=roles,
        show_advanced_settings=is_super_admin
    )
```

---

## Integration Points

### 1. admin_page.py
- Login handling now uses `handle_oauth_login()` and `handle_local_login()`
- Removed old `authenticate()` and `extract_roles()` functions
- Session creation uses new `create_session()` function
- Ready for role-based authorization decorators

### 2. howru.py 
- OAuth callback now uses `handle_oauth_login()` for consistent processing
- Roles are extracted automatically during callback
- Cleaner, more maintainable flow

### 3. Action Handlers
All actions that should be role-restricted can now use:
```python
from api.login_handler import check_authorization

# In action handler:
if action_type == "sensitive_action":
    check_authorization("admin")
    # ... proceed ...
```

---

## Migration Guide

If you have other parts of code checking roles:

### Before
```python
# Roles were not tracked!
user_data = session.get("user")
username = user_data.get("username")  # Only way to identify user
```

### After
```python
from api.login_handler import get_current_user, get_user_roles

user = get_current_user()
username = user.get("username")
roles = get_user_roles()

if "admin" in roles:
    # Do admin things
```

---

## Logging

All authentication and authorization events are logged with appropriate levels:

- `INFO`: User login/logout with roles
- `WARNING`: Failed login attempts, unauthorized access
- `ERROR`: Token processing errors

Example log output:
```
User 'john.doe' logged in via keycloak. Roles: ['admin', 'operator']
Unauthorized access - user 'jane.doe' lacks required role 'admin'
Session cleared for user 'john.doe'
```

---

## Testing the Changes

### Test Local Login
1. Navigate to admin page
2. Enter local credentials
3. Verify: user.roles = ["admin"]
4. Check logs: "User logged in locally"

### Test OAuth Login (Keycloak)
1. Click OAuth login button
2. Authenticate with provider
3. Verify: user.roles = extracted from token
4. Check logs: "User logged in via keycloak. Roles: [...]"

### Test Role-Based Access
1. Create route with `@require_admin` decorator
2. Try accessing as non-admin user → Should get 403
3. Try accessing as admin user → Should succeed

---

## Role Naming Convention

Recommended role names:
- `admin` - Full administrative access
- `operator` - Can execute plugins, restart services
- `viewer` - Read-only access
- `auditor` - Can view logs

These are flexible - use what makes sense for your deployment.

---

## Next Steps

Consider these enhancements:

1. **Database of Users & Roles** - Store user-role mapping locally
2. **Role-Based Template Rendering** - Show/hide UI elements based on roles
3. **Audit Trail** - Log all sensitive operations with user/role
4. **Permission Matrix** - Define which actions require which roles
5. **API Token Authentication** - For programmatic access with role enforcement

---

## Summary

✅ **Cleaner Code** - Login logic extracted to dedicated module  
✅ **Role-Aware** - All users now have roles in session  
✅ **Consistent** - Same login flow for local and OAuth  
✅ **Authorizable** - Decorators for easy role enforcement  
✅ **Maintainable** - Separation of concerns  
✅ **Well-Logged** - All auth events logged
