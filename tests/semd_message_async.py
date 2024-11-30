import asyncio
import time


async def send_message(message: str, host: str = '127.0.0.1', port: int = 6379):
    reader, writer = await asyncio.open_connection(host, port)
    print(f'Sending: {message}')
    writer.write(message.encode())
    await writer.drain()

    data = await reader.read(1024)
    print(f'Received: {data.decode()}')

    writer.close()
    await writer.wait_closed()


async def main():
    messages = [
        "+hello\r\n",
        "$5\r\nworld\r\n",
        "test message\n",
    ]

    tasks = [send_message(msg) for msg in messages]
    await asyncio.gather(*tasks)


if __name__ == "__main__":
    start = time.time()
    asyncio.run(main())
    print(f"Total time: {time.time() - start:.2f}s")
