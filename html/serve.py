#!/usr/bin/env python3
"""Simple dev server for testing the WASM solver.
Serves static files and executes *.cgi scripts as CGI.
Run from the html/ directory: python3 serve.py [port]
"""
import http.server
import sys
import os

class Handler(http.server.CGIHTTPRequestHandler):
    def is_cgi(self):
        if self.path.split('?')[0].endswith('.cgi'):
            self.cgi_info = '', self.path[1:]
            return True
        return False

if __name__ == '__main__':
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    with http.server.HTTPServer(('', port), Handler) as httpd:
        print(f'Serving http://localhost:{port}/?puz=b10x10s1')
        httpd.serve_forever()
