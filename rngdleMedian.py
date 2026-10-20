import requests, json
import numpy as np
import matplotlib.pyplot as plt
user = input("Enter rngdle username: ")

url = "https://www.rngdle.com/api/users/" + user + "/rolls"

querystring = {"limit":"100","offset":"0"}


payload = ""
headers = {
    "Accept": "*/*",
    "Accept-Encoding": "gzip, deflate, br, zstd",
    "Accept-Language": "en-US,en;q=0.9",
    "Alt-Used": "www.rngdle.com",
    "Connection": "keep-alive",
    "Cookie": "__Secure-better-auth.session_token=kgrcbKr493nHwl74v6U1NIPTGySSZasT.Ptj5RT4PUqvZQX4r17E%2F8obcOnpCdNADHlN7eFzoW20%3D; rngdle_user=eyJpZCI6IjEyNzA3MDM4OWRjZDNkZDhhOWExNWMzMjE0ZjM1M2FmIiwiZGlzcGxheU5hbWUiOiJsYXVlIiwiZGlzcGxheU5hbWVDb2xvciI6IiMwMDAwMDAiLCJpc0FkbWluIjpmYWxzZSwiY2FuQWNjZXNzRXhwZXJpbWVudHMiOmZhbHNlfQ; __Secure-better-auth.session_data=eyJzZXNzaW9uIjp7InNlc3Npb24iOnsiZXhwaXJlc0F0IjoiMjAyNi0xMS0xM1QxNjo1MzoxOS4yOTFaIiwidG9rZW4iOiJrZ3JjYktyNDkzbkh3bDc0djZVMU5JUFRHeVNTWmFzVCIsImNyZWF0ZWRBdCI6IjIwMjYtMDktMTBUMTc6MDI6NDIuMTg1WiIsInVwZGF0ZWRBdCI6IjIwMjYtMTAtMTRUMTY6NTM6MTkuMjkxWiIsImlwQWRkcmVzcyI6ImVhZmRmZmZhZmYzMDUzYWM3Y2RhNTI0MTExNGZjODZlMjE5YjA2NGE2YTRiNWMyYjQzZGRjNDY3Zjg1N2E4N2IiLCJ1c2VyQWdlbnQiOiJhNzhlMGYwNWE3NTg1NzJhYmFhMmJjMDc5ZWRhOWU1NWJmMGU2ZDRjYWZkZTY3MTg2N2YxOGM2Yzg4ZGRhMzc0IiwidXNlcklkIjoiMTI3MDcwMzg5ZGNkM2RkOGE5YTE1YzMyMTRmMzUzYWYiLCJpZCI6ImtFd1NDTGphRkhBaVExem9yQkdNZjJYS0dwbEhFbVd2In0sInVzZXIiOnsibmFtZSI6bnVsbCwiZW1haWwiOiI2ZWE2NTljZjcxZTA5OWE3NmY0NmY5NjY4MTEwMGU5M2E2OGIxMzJkMjMxODY1ZTI5MGQwOTYxMDNmNGJmYjMyIiwiZW1haWxWZXJpZmllZCI6dHJ1ZSwiaW1hZ2UiOm51bGwsImNyZWF0ZWRBdCI6IjIwMjYtMDktMTBUMTc6MDI6NDIuMDU3WiIsInVwZGF0ZWRBdCI6IjIwMjYtMDktMTBUMTc6MDM6NDEuNzgwWiIsImRpc3BsYXlOYW1lIjoibGF1ZSIsImRpc3BsYXlOYW1lQ29sb3IiOiIjMDAwMDAwIiwiY2FuU2V0Q29sb3IiOnRydWUsImlzQWRtaW4iOmZhbHNlLCJkYWlseVJvbGxMaW1pdEV4ZW1wdCI6ZmFsc2UsImNhbkFjY2Vzc0V4cGVyaW1lbnRzIjpmYWxzZSwiaWQiOiIxMjcwNzAzODlkY2QzZGQ4YTlhMTVjMzIxNGYzNTNhZiJ9LCJ1cGRhdGVkQXQiOjE3OTIxMTY4Nzc1NjgsInZlcnNpb24iOiIzIn0sImV4cGlyZXNBdCI6MTc5MjIwMzI3NzU2OCwic2lnbmF0dXJlIjoiek9VTWJ3UFJuYzFzVUdPd1hGcnpkdkt0Vk9MZkhfa1RKZTlkOHJOWWs4OCJ9",
    "DNT": "1",
    "Priority": "u=0",
    "Referer": ("https://www.rngdle.com/u/" + user),
    "Sec-Fetch-Dest": "empty",
    "Sec-Fetch-Mode": "cors",
    "Sec-Fetch-Site": "same-origin",
    "Sec-GPC": "1",
    "TE": "trailers",
    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:157.0) Gecko/20100101 Firefox/157.0"
}

response = requests.request("GET", url, data=payload, headers=headers, params=querystring)

# print(response.text)
print(response)
if response != requests.Response(200):
    raise("User Does Not Exist")

data = json.loads(response.text)
rolls = []
if data != None:
    for roll in data["rolls"]:
        # print(roll)
        rolls.append(roll["totalScore"])

print("Median: " + str(np.median(rolls)))
print("Mean: "+ str(np.mean(rolls)))
# print(sorted(rolls))

data = np.array(rolls)

data_min = data.min()
data_max = data.max()

# 3. Create adjusting buckets (e.g., 5 equal buckets from min to max)
num_buckets = 10
buckets = np.linspace(data_min, data_max, num_buckets + 1)

# 4. Calculate frequencies using NumPy's histogram function
counts, edges = np.histogram(data, bins=buckets)

print("\n=== TERMINAL BAR CHART ===")
print(f"Data Range: {data_min} to {data_max}\n")

# 5. Print the ASCII plot line by line
# 'max_width' keeps the bar length under control if counts get massive
max_width = 30 
max_count = max(counts) if max(counts) > 0 else 1

for i in range(num_buckets):
    # Format the bucket range label (e.g., "12.0 - 29.4")
    label = f"[{edges[i]:6.1f} : {edges[i+1]:6.1f}]"
    
    # Scale the bar length proportionally
    bar_length = int((counts[i] / max_count) * max_width)
    bar = "█" * bar_length
    
    # Print the bucket, the visual bar, and the exact count value
    print(f"{label} | {bar:<{max_width}} ({counts[i]})")

print("=" * 50)


q1, q3 = np.percentile(data, [25, 75])
iqr = q3 - q1
lower_bound = q1 - 1.5 * iqr
upper_bound = q3 + 1.5 * iqr

# Create the boolean mask
outlier_mask = (data < lower_bound) | (data > upper_bound)

# Get the outliers
outliers = data[outlier_mask]

# Get the clean data (by inverting the mask with ~)
clean_data = data[~outlier_mask]

print("Outliers:", outliers)      # Output: [-50 102]