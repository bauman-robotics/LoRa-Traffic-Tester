from flask import Flask, request, jsonify
import psycopg2
import os
from datetime import datetime

app = Flask(__name__)

def get_db_connection():
    return psycopg2.connect(
        host=os.environ.get('DB_HOST', 'localhost'),
        database=os.environ.get('DB_NAME', 'lora_db'),
        user=os.environ.get('DB_USER', 'lora_user'),
        password=os.environ.get('DB_PASS', 'lora_pass')
    )

@app.route('/lora', methods=['POST'])
def receive_lora_data():
    try:
        data = request.get_json()

        # Извлечение данных
        payload = data.get('data', '')
        rssi = data.get('rssi')
        snr = data.get('snr')
        frequency = data.get('frequency')
        bandwidth = data.get('bandwidth')
        sf = data.get('spreading_factor')

        # Сохранение в базу данных
        conn = get_db_connection()
        cur = conn.cursor()

        cur.execute(
            """
            INSERT INTO lora_packets (data, rssi, snr, frequency, bandwidth, spreading_factor)
            VALUES (%s, %s, %s, %s, %s, %s)
            """,
            (payload, rssi, snr, frequency, bandwidth, sf)
        )

        conn.commit()
        cur.close()
        conn.close()

        return jsonify({'status': 'success'}), 200

    except Exception as e:
        print(f"Error: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

@app.route('/health', methods=['GET'])
def health_check():
    return jsonify({'status': 'healthy'}), 200

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
