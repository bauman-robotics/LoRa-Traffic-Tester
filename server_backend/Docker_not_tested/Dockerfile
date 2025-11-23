FROM python:3.9-slim

WORKDIR /app

COPY requirements_server.txt .
RUN pip install -r requirements_server.txt

COPY data_collector.py .

EXPOSE 5000

FROM python:3.9-slim
