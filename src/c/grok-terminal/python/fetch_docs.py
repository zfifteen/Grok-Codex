import urllib.request
from bs4 import BeautifulSoup

url = "https://www.jetbrains.com/help/idea/mcp-server.html"
try:
    with urllib.request.urlopen(url) as response:
        html = response.read().decode('utf-8')
    soup = BeautifulSoup(html, 'html.parser')
    text = soup.get_text()
    print(text)
except Exception as e:
    print(f"Error: {e}")