#!/usr/bin/python3
import json
import hashlib
import random
import string

CH_MAX = 35
ALPHA = list("abcdefghijklmnopqrstuvwxyz!%{}@?&^=")

class User:
    def __init__(self, first_name: str, last_name: str):
        self.first_name = first_name
        self.last_name = last_name

    def __eq__(self, other):
        return self.first_name == other.first_name and self.last_name == other.last_name

    def __hash__(self):
        return hash((self.first_name, self.last_name))

def count_alphas(s: str) -> int:
    return sum(1 for c in s if c.isalpha())

def random_str(ch: int) -> str:
    return ''.join(random.choice(ALPHA) for _ in range(ch))


def check_user_exists(username):
    try:
        with open("/etc/almond/tokens", "r") as file:
            for line_num, line in enumerate(file, 1):
                line = line.strip()
                if not line or '\t' not in line:
                    print(f"Warning: Invalid format at line {line_num}")
                    continue
                try:
                    name, token = line.split('\t')
                    if not name or not token:
                        print(f"Warning: Empty fields at line {line_num}")
                        continue
                    if name == username:
                        return token
                except ValueError:
                    print(f"Warning: Invalid tab separation at line {line_num}")
                    continue
        return "None"
    except FileNotFoundError:
        print(f"File {filename} not found")
        return "None"
    except Exception as e:
        print(f"Error reading file: {e}")
        return "None"

def generate_token(first_name: str, last_name: str) -> str:
    if len(first_name) < 2 or count_alphas(first_name) != len(first_name):
        print(f"Your first name ('{first_name}') seems odd.")
        return None
    if len(last_name) < 2 or count_alphas(last_name) != len(last_name):
        print(f"Your last name ('{last_name}') seems odd.")
        return None

    # check if user already have a token
    uname = first_name + " " + last_name
    utok = check_user_exists(uname)
    if (utok != "None"):
        return utok

    user = User(first_name, last_name)
    hashed = hashlib.sha256(f"{user.first_name}{user.last_name}".encode()).hexdigest()
    token = random_str(5) + hashed[:10] + random_str(10)
    
    with open("/etc/almond/tokens", "a") as hFile:
        hFile.write(f"{user.first_name} {user.last_name}\t{token}\n")

    print("Your hash is:", token)
    print("Token added to Almond configuration.")
    return token

def add_token_to_login(user, token, userExist=False):
    lines = []
    foundUser = False
    with open("/etc/almond/users.conf", "r") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                entry = json.loads(line)
                if user in entry:
                    foundUser = True
                    if not userExist:
                        entry["token"] = token
                lines.append(json.dumps(entry))
            except json.JSONDecodeError:
                lines.append(line)
   
    if userExist:
        return foundUser 
    with open ("/etc/almond/users.conf", "w") as f:
        for l in lines:
            f.write(l + "\n")
    print(f"Entry for user '{username}' updated in '/etc/almond/users.conf' .")
        
if __name__ == "__main__":
    import sys
    if len(sys.argv) == 3:
        username, fname, lname = sys.argv[1], sys.argv[2], sys.argv[3]
    else:
        username = input("Your login name: ")
        fname = input("Enter your first name: ")
        lname = input("Enter your last name: ")
    
    if (add_token_to_login(username, "None", True)):
        token = generate_token(fname, lname)
        add_token_to_login(username, token)
    else:
        print ("No user named '" + username + "' found.")

