#!/usr/bin/env python3
import requests
import os
import sys
import time

def test_get_request():
    print("\n=== Test GET Request ===")
    try:
        response = requests.get('http://127.0.0.1:8080/')
        print(f"Status Code: {response.status_code}")
        print(f"Response: {response.text[:100]}...")
        return response.status_code == 200
    except Exception as e:
        print(f"Error: {e}")
        return False

def test_post_request():
    print("\n=== Test POST Request ===")
    try:
        data = {'test': 'data'}
        response = requests.post('http://127.0.0.1:8080/upload', data=data)
        print(f"Status Code: {response.status_code}")
        return response.status_code == 200
    except Exception as e:
        print(f"Error: {e}")
        return False

def test_delete_request():
    print("\n=== Test DELETE Request ===")
    try:
        response = requests.delete('http://127.0.0.1:8080/test.txt')
        print(f"Status Code: {response.status_code}")
        return response.status_code in [200, 204]
    except Exception as e:
        print(f"Error: {e}")
        return False

def test_invalid_method():
    print("\n=== Test Invalid Method ===")
    try:
        response = requests.put('http://127.0.0.1:8080/')
        print(f"Status Code: {response.status_code}")
        return response.status_code == 405
    except Exception as e:
        print(f"Error: {e}")
        return False

def test_not_found():
    print("\n=== Test Not Found ===")
    try:
        response = requests.get('http://127.0.0.1:8080/nonexistent')
        print(f"Status Code: {response.status_code}")
        return response.status_code == 404
    except Exception as e:
        print(f"Error: {e}")
        return False

def main():
    # Attendre que le serveur démarre
    print("Waiting for server to start...")
    time.sleep(2)

    tests = [
        ("GET Request", test_get_request),
        ("POST Request", test_post_request),
        ("DELETE Request", test_delete_request),
        ("Invalid Method", test_invalid_method),
        ("Not Found", test_not_found)
    ]

    success = 0
    total = len(tests)

    for name, test in tests:
        print(f"\nRunning {name}...")
        if test():
            print(f"✓ {name} passed")
            success += 1
        else:
            print(f"✗ {name} failed")

    print(f"\nTest Summary: {success}/{total} tests passed")

if __name__ == "__main__":
    main() 