import io
import os
import json
import pyotp
import qrcode
import socket
from werkzeug.security import check_password_hash
from flask import Blueprint, render_template_string, request, redirect, url_for, session, send_file
from flask import current_app, render_template

auth_blueprint = Blueprint('auth', __name__)

logon_img = '/static/almond.png'
# For demonstration purposes, store user secrets in a dictionary.
# In production, securely store these in your database.
admin_user_file = '/etc/almond/users.conf'
is_container = 'false'
user_secrets = {}

def verify_password(username, password):
    global admin_user_file, users
    users = {}
    print("DEBUG: Verify password for %s with password %s" % (username, password))
    if os.path.isfile(admin_user_file):
        with open(admin_user_file, 'r') as f:
            for line_num, line in enumerate(f, 1):
                print ("DEBUG: ", line.strip())
                try:
                    user_data = json.loads(line.strip())
                    print("DEBUG: ", user_data)
                    #username = list(user_data.keys())[0]
                    #users[username] = user_data[username]
                    for user_key, hash_value in user_data.items():
                        users[user_key] = hash_value
                except json.JSONDecodeError as e:
                    print(f"Warning: Invalid JSON format at line {line_num}: {str(e)}")
                    continue
    else:
        users = {}
    if username in users:
        return check_password_hash(users.get(username), password)
    return False

#def verify_password(username, password):
#    global admin_user_file, users
#    if os.path.isfile(admin_user_file):
#        with open(admin_user_file, 'r') as f:
#            for line_num, line in enumerate(f, 1):
#                try:
#                    user_data = json.loads(line.strip())
#                    username = list(user_data.keys())[0]
#                    users[username] = user_data[username]
#                except json.JSONDecodeError as e:
#                    print(f"Warning: Invalid JSON format at line {line_num}: {str(e)}")
#                    continue
#    else:
#        users = {}
#    if username in users:
#        return check_password_hash(users.get(username), password)
#    return False

def generate_qr_code(data):
    qr = qrcode.QRCode(
        version=1,
        error_correction=qrcode.constants.ERROR_CORRECT_H,  # High error correction
        box_size=10,  # Increase box_size to create a larger image
        border=4,
    )
    qr.add_data(data)
    qr.make(fit=True)
    img = qr.make_image(fill='black', back='white')
    return img

@auth_blueprint.route('/almond/admin/enable_2fa/<username>')
def enable_2fa(username):
    global issuer
    # Generate a new TOTP secret for the user
    user_secret = pyotp.random_base32()
    user_secrets[username] = user_secret
    print("User_secret:", user_secret)
    print("Debug - Secret key for {}: {}".format(username, user_secret))

    # Create the provisioning URI for the authenticator app
    totp = pyotp.TOTP(user_secret)
    issuer = "howru-"
    is_container = current_app.config['IS_CONTAINER']
    if (is_container == 'true'):
        config = {}
        with open("/etc/almond/almond.conf", "r") as conf:
            for line in conf:
                key, value = parse_line(line)
                config[key] = value
        issuer = issuer + config.get('scheduler.hostName', socket.gethostname())
        print("DEBUG: IS containerized")
    else:
        issuer = issuer + socket.gethostname()
    provisioning_uri = totp.provisioning_uri(name=username, issuer_name=issuer)
    #print("Provisioning URI:", provisioning_uri)

    # Render a simple HTML page with the QR code image embedded
    a_auth_type = current_app.config['AUTH_TYPE']
    html = '''
        <h1>Enable Two-Factor Authentication for {{ username }}</h1>
        <p>Scan this QR code with your authenticator app:</p>
        <img src="{{ url_for('auth.qr_code', username=username, issuer=issuer) }}" alt="QR Code">
        <hr>
        <h2>Manual Entry</h2>
        <p>If you cannot scan the QR code, enter these details into your authenticator app:</p>
        <ul>
          <li><strong>Issuer:</strong> howru</li>
          <li><strong>Account Name:</strong> {{ username }}</li>
          <li><strong>Secret Key:</strong> {{ user_secret }}</li>
          <li><strong>Algorithm:</strong> SHA1</li>
          <li><strong>Digits:</strong> 6</li>
          <li><strong>Period:</strong> 30 seconds</li>
        </ul>
    '''
    if (a_auth_type == "2fa"):
        #return render_template_string(html, username=username, user_secret=user_secret)
        return render_template('enablefa.html', logon_image=logon_img, username=username, user_secret=user_secret, issuer=issuer)
    else:
        return render_template("403_fa.html")    

@auth_blueprint.route('/almond/admin/qr_code/<username>')
def qr_code(username):
    global issuer
    a_auth_type = current_app.config['AUTH_TYPE']
    if (a_auth_type != "2fa"):
        return render_template("403_fa.html")
    user_secret = user_secrets.get(username)
    if not user_secret:
        return "User not found or 2FA not enabled.", 404
    
    totp = pyotp.TOTP(user_secret)
    provisioning_uri = totp.provisioning_uri(name=username, issuer_name=issuer)
    #qr = qrcode.make(provisioning_uri)
    qr = generate_qr_code(provisioning_uri)
    
    buf = io.BytesIO()
    qr.save(buf, format='PNG')
    buf.seek(0)
    return send_file(buf, mimetype='image/png')

@auth_blueprint.route('/almond/admin/login', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        username = request.form.get("uname")
        password = request.form.get("psw")
       
        print("DEBUG username/password = %s %s" % (username, password))  
        if verify_password(username, password):
            session['username'] = username
            # Redirect to 2FA verification step
            return redirect(url_for('auth.verify_2fa'))
        else:
            return "Invalid username or password", 401

    # Simple login form
    return render_template('login_fa.html', logon_image=logon_img)

@auth_blueprint.route('/verify_2fa', methods=['GET', 'POST'])
def verify_2fa():
    username = session.get('username')
    if not username:
        return redirect(url_for('auth.login'))

    if request.method == 'POST':
        token = request.form.get('token')
        user_secret = user_secrets.get(username)
        print ("DEBUG:\n")
        print (user_secret)
        if user_secret:
            totp = pyotp.TOTP(user_secret)
            if totp.verify(token):
                session['authenticated'] = True
                session['login'] = 'true'
                session['user'] = username
                #return f"Welcome, {username}! You are fully logged in."
                return redirect('/almond/admin')                
            else:
                return "Invalid 2FA token.", 401
        else:
            return "2FA is not enabled for this account.", 400

    #return '''
    #    <h1>Two-Factor Authentication</h1>
    #    <form method="post">
    #        Enter your 2FA token: <input name="token" type="text"><br>
    #        <input type="submit" value="Verify">
    #    </form>
    #'''
    return render_template('verify.html', logon_image=logon_img, username=username)

@auth_blueprint.route('/protected')
def protected():
    if not session.get('authenticated'):
        return redirect(url_for('auth.login'))
    return "This is a protected page accessible only to fully authenticated users."
