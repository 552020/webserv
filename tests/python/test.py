import requests
from concurrent.futures import ThreadPoolExecutor


# Function to send a request
def send_request(url):
    try:
        response = requests.get(url)
        print(f"Response from {url}: {response.status_code}")
    except requests.exceptions.RequestException as e:
        print(f"Request to {url} failed: {e}")


# URL to which the requests will be sent
url = "http://localhost:8080"

# Number of concurrent requests
num_requests = 10000

# Creating a ThreadPoolExecutor
with ThreadPoolExecutor(max_workers=num_requests) as executor:
    # Submitting tasks to the executor
    futures = [executor.submit(send_request, url) for _ in range(num_requests)]

# The script will print the response status code or an error for each request
