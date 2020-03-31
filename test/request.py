#!/usr/bin/env python3
import sys
import requests

def main(port):
    url = 'http://localhost:' + str(port) + '/../test/index.html'
    print("url: {}".format(url))
    response = requests.get(url)
    print(response)
    print(response.headers)
    print(response.text)

if __name__ == '__main__':
    port = 8001
    if len(sys.argv) > 1:
        port = argv[1]
    main(port)
