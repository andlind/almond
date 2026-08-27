#!/usr/bin/python3
import json
import os
import time
from werkzeug.security import check_password_hash, generate_password_hash

admin_user_file = '/etc/almond/users.conf'
users = {}

def hash_password(password):
    """Hash a password for secure storage"""
    return hashlib.sha256(password.encode()).hexdigest()

def validate_username(username):
    """Validate username format"""
    return len(username) >= 3 and username.isalnum()

def validate_password(password):
    """Validate password strength"""
    return len(password) >= 8 and any(c.isdigit() for c in password) and any(c.isupper() for c in password)

def verify_password(username, password):
    global admin_user_file, users
    users = {}
    if os.path.isfile(admin_user_file):
        with open(admin_user_file, 'r') as f:
            for line_num, line in enumerate(f, 1):
                try:
                    user_data = json.loads(line.strip())
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

def add_user():
    """Add a new user to the system"""
    global admin_user_file
    username = input("\nEnter username (minimum 3 characters, alphanumeric): ").strip()
    
    # Validate username
    if not validate_username(username):
        print("Invalid username! Must be at least 3 characters and contain only letters and numbers.")
        return
    
    # Check if username exists
    if check_username_exists(username):
        print("Username already exists!")
        return
    
    password = input("Enter password (minimum 8 characters, must contain uppercase and digit): ")
    
    # Validate password
    if not validate_password(password):
        print("Invalid password! Must be at least 8 characters and contain uppercase letter and digit.")
        return
    
    user = {
        username: generate_password_hash(password.strip())
    }
    
    # Create file with header if it doesn't exist
    if not os.path.exists(admin_user_file):
        with open(admin_user_file, "w") as f:
            f.write(json.dumps(user))
            f.write("\n")
    else:    
        with open(admin_user_file, 'a') as f:
            f.write(json.dumps(user))
            f.write("\n")
    
    print("\nUser created successfully!")
    time.sleep(1)

def check_username_exists(username):
    """Check if username exists"""
    global admin_user_file
    if not os.path.exists(admin_user_file):
        return False
    
    with open(admin_user_file, 'r') as f:
        for line_num, line in enumerate(f, 1):
            try:
                user_data = json.loads(line.strip())
                storedname = list(user_data.keys())[0]
                if (username == storedname):
                     return True
            except json.JSONDecodeError as e:
                print(f"Warning: Invalid JSON format at line {line_num}: {str(e)}")
                continue
    return False

def login():
    """Login functionality"""
    global admin_user_file
    username = input("\nEnter username: ").strip()
    password = input("Enter password: ")
    
    if verify_password(username, password):
        print ("LOGIN SUCCESFULL!\n")
    else:
        print ("LOGIN FAILED!\n")

def display_users():
    """Display all users"""
    global admin_user_files 
    d_users = {}
    if not os.path.exists(admin_user_file):
        print("No users exist yet!")
        return

    with open(admin_user_file, 'r') as f:
       for line_num, line in enumerate(f, 1):
           try:
               user_data = json.loads(line.strip())
               username = list(user_data.keys())[0]
               d_users[username] = user_data[username]
           except json.JSONDecodeError as e:
                print(f"Warning: Invalid JSON format at line {line_num}: {str(e)}")
                continue
    usernames_list = list(d_users.keys())
    usernames_list.sort()
    for user in usernames_list:
        print(user)

def delete_user_ex(username):
    with open(admin_user_file, 'r') as file:
        lines = file.readlines()
    filtered_lines = [line for line in lines if username.lower() not in line.lower()]
    with open(admin_user_file, 'w') as file:
        file.writelines(filtered_lines)

def delete_user():
    d_users = {}
    with open(admin_user_file, 'r') as f:
       for line_num, line in enumerate(f, 1):
           try:
               user_data = json.loads(line.strip())
               username = list(user_data.keys())[0]
               d_users[username] = user_data[username]
           except json.JSONDecodeError as e:
                print(f"Warning: Invalid JSON format at line {line_num}: {str(e)}")
                continue
    usernames_list = list(d_users.keys())
    deluser = input("Enter user to be removed: ")
    if deluser in usernames_list and deluser != "admin":
        delete_user_ex(deluser)
        print("Removed user ", deluser)
    else:
        print("Removing user failed.")

def main():
    """Main program loop"""
    while True:
        print("\n=== User Management System ===")
        print("1. Add User")
        print("2. Test login")
        print("3. Display Users")
        print("4. Delete User")
        print("5. Exit")
        
        choice = input("\nEnter your choice (1-5): ").strip()
        
        if choice == '1':
            add_user()
        elif choice == '2':
            login()
        elif choice == '3':
            display_users()
        elif choice == '4':
            delete_user()
        elif choice == '5':
            print("\nGoodbye!")
            break
        else:
            print("\nInvalid choice! Please select 1-4.")
        
        time.sleep(1)

if __name__ == "__main__":
    main()
