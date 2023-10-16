
with open("out.bag", "rb") as file:
    chunk_data = file.read(100000000)
print(chunk_data[:])
