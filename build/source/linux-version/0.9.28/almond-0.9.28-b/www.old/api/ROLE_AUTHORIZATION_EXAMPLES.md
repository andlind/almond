# Role-Based Authorization Examples

This file shows practical examples of how to use the new login handler
for role-based access control in admin_page.py

## Example 1: Protect Sensitive Actions with Inline Checks

```python
# In admin_page.py, in the action handler section:

if action_type == 'restart_almond':
    from api.login_handler import check_authorization, get_current_user
    
    check_authorization("admin")  # Raises 403 if user doesn't have admin role
    
    user = get_current_user()
    logger.info(f"User '{user['username']}' (roles: {user['roles']}) restarting almond")
    
    # ... existing restart code ...
```

## Example 2: Add Multiple Authorization Levels

```python
from api.login_handler import check_authorization

if action_type == 'execute_plugin':
    # Operators or admins can execute plugins
    check_authorization(["operator", "admin"])
    
    user = get_current_user()
    logger.info(f"User '{user['username']}' executing plugin")
    
    # ... execution code ...

elif action_type == 'delete_plugin':
    # Only admins can delete
    check_authorization("admin")
    logger.info(f"User '{user['username']}' deleting plugin")
    # ... deletion code ...
```

## Example 3: Create Admin-Only Routes

```python
from api.login_handler import require_admin, require_any_role

@admin_page.route('/almond/admin/config', methods=['GET'])
@require_admin
def admin_config():
    # Only admins can access admin config page
    # Returns 403 for non-admin users
    return render_template('admin_config.html')

@admin_page.route('/almond/admin/restart', methods=['POST'])
@require_any_role('admin', 'operator')
def restart_service():
    # Admins or operators can restart
    user = get_current_user()
    logger.info(f"Service restart by {user['username']} (roles: {user['roles']})")
    # ... restart code ...
    return {"status": "restarted"}
```

## Example 4: Conditional Functionality Based on Role

```python
from api.login_handler import get_user_roles, get_current_user

def render_admin_page():
    user = get_current_user()
    roles = get_user_roles()
    
    # Prepare data based on user's roles
    can_restart = "admin" in roles
    can_execute = "operator" in roles or "admin" in roles
    can_view_logs = True  # All logged-in users
    can_edit_config = "admin" in roles
    
    context = {
        "user": user,
        "roles": roles,
        "can_restart": can_restart,
        "can_execute": can_execute,
        "can_view_logs": can_view_logs,
        "can_edit_config": can_edit_config,
    }
    
    return render_template('admin.html', **context)
```

## Example 5: Log Sensitive Operations with Roles

```python
from api.login_handler import check_authorization, get_current_user

if action_type == 'update_user_password':
    check_authorization("admin")
    
    user = get_current_user()
    target_user = request.form['username']
    
    logger.warning(
        f"SECURITY: Admin '{user['username']}' "
        f"changed password for user '{target_user}' "
        f"(admin roles: {user['roles']})"
    )
    
    # ... password update code ...
```

## Example 6: Audit Trail with Role Information

```python
from api.login_handler import get_current_user

def audit_log(action, resource, result):
    """Log an action with user and role information"""
    user = get_current_user()
    if user:
        logger.info(
            f"AUDIT: user='{user['username']}' "
            f"roles={user['roles']} "
            f"action='{action}' "
            f"resource='{resource}' "
            f"result='{result}'"
        )
    else:
        logger.warning(f"Audit log called without user context: {action}")

# Usage:
if action_type == 'restart_almond':
    check_authorization("admin")
    try:
        result = restart_almond_service()
        audit_log("restart_service", "almond", "success")
    except Exception as e:
        audit_log("restart_service", "almond", f"failed: {e}")
```

## Example 7: Template-Level Authorization

In your Jinja2 templates (e.g., admin.html):

```html
<!-- Show restart button only if user can restart -->
{% if can_restart %}
    <button onclick="restartService()">Restart Almond</button>
{% endif %}

<!-- Show admin panel only for admins -->
{% if 'admin' in roles %}
    <div class="admin-panel">
        <h3>Advanced Configuration</h3>
        <!-- admin-only content -->
    </div>
{% endif %}

<!-- Show different content based on roles -->
{% if 'operator' in roles %}
    <div class="operator-dashboard">
        <!-- operator-specific dashboard -->
    </div>
{% elif 'viewer' in roles %}
    <div class="viewer-dashboard">
        <!-- read-only dashboard -->
    </div>
{% endif %}
```

## Example 8: API Token-Based Authorization (Future)

Once you add API token support:

```python
from api.login_handler import check_authorization, get_current_user
from functools import wraps

def require_token_with_role(required_role):
    """Decorator for API endpoints that need token + role"""
    def decorator(f):
        @wraps(f)
        def decorated_function(*args, **kwargs):
            # Extract token from header
            token = request.headers.get('Authorization')
            if not token:
                return {"error": "Missing token"}, 401
            
            # Validate token and check role
            try:
                check_authorization(required_role)
            except:
                return {"error": "Unauthorized"}, 403
            
            return f(*args, **kwargs)
        return decorated_function
    return decorator

@app.route('/api/almond/restart', methods=['POST'])
@require_token_with_role('admin')
def api_restart_almond():
    """API endpoint to restart almond - admin only"""
    return {"status": "restarted"}
```

## Example 9: Progressive Enhancement

Add role checking to existing code gradually:

```python
# STEP 1: Add basic login requirement
from api.login_handler import require_login

@admin_page.route('/almond/admin', methods=['GET', 'POST'])
@require_login  # Ensure user is logged in
def index():
    # ... existing code ...

# STEP 2: Later, add role requirements for specific actions
if action_type == 'restart_almond':
    from api.login_handler import check_authorization
    check_authorization("admin")  # Add this line
    # ... existing restart code ...

# STEP 3: Even later, convert to decorators
@require_admin  # Use decorator for new routes
def admin_restart_endpoint():
    # ... restart code ...
```

## Example 10: Comprehensive Example

```python
from api.login_handler import (
    check_authorization,
    get_current_user,
    get_user_roles,
    user_has_role,
)

# In admin_page.py index() function:

if action_type == 'api':
    """Update API configuration - admin only"""
    check_authorization("admin")
    
    user = get_current_user()
    roles = get_user_roles()
    
    logger.info(
        f"User '{user['username']}' (roles: {roles}) "
        f"updating API configuration"
    )
    
    # Parse and update configuration
    update_lines = []
    for key, val in request.form.items():
        if not key == "action_type":
            line = key + "=" + val
            update_lines.append(line)
    
    if update_lines:
        write_conf = rewrite_config(api_conf_file, update_lines)
        logger.info(f"Configuration updated by '{user['username']}'")
    
    return render_template('howruconf.html',
        conf=write_conf,
        user_image=image_file,
        avatar=almond_avatar,
        info=f"Config updated by {user['username']}"
    )

elif action_type == 'execute_plugin':
    """Execute plugin - operator or admin"""
    check_authorization(["operator", "admin"])
    
    user = get_current_user()
    pid = request.form['plugin_id']
    
    logger.info(
        f"User '{user['username']}' (roles: {user['roles']}) "
        f"executing plugin {pid}"
    )
    
    # Execute plugin...
    exr = execute_plugin_object(pid)
    
    if exr == 0:
        audit_log("execute_plugin", pid, "success")
    else:
        audit_log("execute_plugin", pid, "failed")
    
    # ... rest of code ...
```

---

## Migration Checklist

When adding role-based authorization to your code:

- [ ] Import required functions from `login_handler`
- [ ] Add authorization checks to sensitive actions
- [ ] Update logging to include user roles
- [ ] Test with different user roles
- [ ] Update templates to show/hide based on roles
- [ ] Document role requirements for each action
- [ ] Create audit trail entries for sensitive operations
- [ ] Verify error handling (403 responses)

---

## Common Patterns

### Pattern 1: Action Guard
```python
if action_type == 'dangerous_action':
    check_authorization("admin")
    # ... proceed with dangerous operation ...
```

### Pattern 2: Role-Based Logic
```python
user = get_current_user()
if user_has_role("admin"):
    # admin code path
elif user_has_role("operator"):
    # operator code path
else:
    # viewer code path
```

### Pattern 3: Multiple Authorization Levels
```python
# Try highest privilege first
if user_has_role("admin"):
    full_access = True
elif user_has_role("operator"):
    limited_access = True
else:
    check_authorization("viewer")  # Raise if not viewer
    read_only = True
```

### Pattern 4: Log All Admin Actions
```python
user = get_current_user()
roles = get_user_roles()
if "admin" in roles:
    logger.warning(f"ADMIN ACTION by {user['username']}: {action_type}")
```
