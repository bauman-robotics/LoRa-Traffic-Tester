# Перезагрузите Nginx:
# sudo nginx -t
# sudo systemctl reload nginx

# sudo systemctl daemon-reload
# sudo systemctl start lora-api.service
# sudo systemctl stop lora-api.service
# sudo systemctl status lora-api.service
# sudo systemctl restart lora-api.service

#Если запускаете python lora_api.py, логи будут выводиться прямо в консоль.

# sudo systemctl daemon-reload
# sudo systemctl restart lora-api.service
# python3 lora_api.py

from flask import Flask, request, jsonify
import psycopg2
from psycopg2.extras import RealDictCursor
import json
from datetime import datetime
from flask import render_template_string
from config import DB_CONFIG, DEBUG, HOST, PORT

app = Flask(__name__)

def get_db_connection():
    """Создает подключение к базе данных"""
    return psycopg2.connect(**DB_CONFIG)

@app.route('/api/lora', methods=['GET'])
def get_all_data():
    """Получить все записи из таблицы lora_tab"""
    try:
        conn = get_db_connection()
        cur = conn.cursor(cursor_factory=RealDictCursor)
        
        cur.execute("""
            SELECT * FROM lora_tab 
            ORDER BY created_at DESC
        """)
        
        data = cur.fetchall()
        cur.close()
        conn.close()
        
        return jsonify({
            'status': 'success',
            'count': len(data),
            'data': data
        })
        
    except Exception as e:
        return jsonify({'status': 'error', 'message': str(e)}), 500

@app.route('/api/lora/<int:line_num>', methods=['GET'])
def get_single_data(line_num):
    """Получить одну запись по line_num"""
    try:
        conn = get_db_connection()
        cur = conn.cursor(cursor_factory=RealDictCursor)
        
        cur.execute("SELECT * FROM lora_tab WHERE line_num = %s", (line_num,))
        data = cur.fetchone()
        
        cur.close()
        conn.close()
        
        if data:
            return jsonify({'status': 'success', 'data': data})
        else:
            return jsonify({'status': 'error', 'message': 'Record not found'}), 404
            
    except Exception as e:
        return jsonify({'status': 'error', 'message': str(e)}), 500

@app.route('/api/lora', methods=['POST'])
def add_data():
    """Добавить новую запись или батч записей в таблицу lora_tab"""
    try:
        data = request.json

        if not data:
            return jsonify({'status': 'error', 'message': 'No data provided'}), 400

        conn = get_db_connection()
        cur = conn.cursor()

        # Проверяем, пришел ли батч (массив) или одиночный объект
        if isinstance(data, list):
            # Обработка батча записей - помечаем как батч
            inserted_count = 0
            for item in data:
                # Создаем копию item и устанавливаем additional_field4 = 1 для батчевых записей
                item_copy = item.copy()
                item_copy['additional_field4'] = 1  # 1 = batch, 0 = single, NULL = unknown

                query = """
                    INSERT INTO lora_tab
                    (user_id, user_location, cold, hot, alarm_time,
                     destination_nodeid, sender_nodeid, packet_id, header_flags,
                     channel_hash, next_hop, relay_node, packet_data,
                     signal_level_dbm, full_packet_len, additional_field3, additional_field4)
                    VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
                """

                parameters = (
                    item_copy.get('user_id'),
                    item_copy.get('user_location'),
                    item_copy.get('cold'),
                    item_copy.get('hot'),
                    item_copy.get('alarm_time'),
                    item_copy.get('destination_nodeid'),  # TEXT для HEX
                    item_copy.get('sender_nodeid'),       # TEXT для HEX
                    item_copy.get('packet_id'),
                    item_copy.get('header_flags'),
                    item_copy.get('channel_hash'),
                    item_copy.get('next_hop'),
                    item_copy.get('relay_node'),
                    item_copy.get('packet_data'),
                    item_copy.get('signal_level_dbm'),
                    item_copy.get('full_packet_len'),
                    item_copy.get('additional_field3'),
                    item_copy.get('additional_field4')
                )

                try:
                    cur.execute(query, parameters)
                    inserted_count += 1
                    print(f"DEBUG: Inserted item {inserted_count}")
                except Exception as e:
                    print(f"DEBUG: Error inserting item {inserted_count}: {e}")
                    print(f"DEBUG: Parameters: {parameters}")
                    raise

            conn.commit()
            cur.close()
            conn.close()

            return jsonify({
                'status': 'success',
                'message': f'Batch data added successfully: {inserted_count} records',
                'records_inserted': inserted_count
            })

        else:
            # Обработка одиночного запроса - помечаем как одиночный
            # Создаем копию и устанавливаем additional_field4 = 0 для одиночных записей
            data_copy = data.copy()
            data_copy['additional_field4'] = 0  # 0 = single, 1 = batch, NULL = unknown

            query = """
                INSERT INTO lora_tab
                (user_id, user_location, cold, hot, alarm_time,
                 destination_nodeid, sender_nodeid, packet_id, header_flags,
                 channel_hash, next_hop, relay_node, packet_data,
                 signal_level_dbm, full_packet_len, additional_field3, additional_field4)
                VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
                RETURNING line_num, created_at
            """

            parameters = (
                data_copy.get('user_id'),
                data_copy.get('user_location'),
                data_copy.get('cold'),
                data_copy.get('hot'),
                data_copy.get('alarm_time'),
                data_copy.get('destination_nodeid'),  # TEXT для HEX
                data_copy.get('sender_nodeid'),       # TEXT для HEX
                data_copy.get('packet_id'),
                data_copy.get('header_flags'),
                data_copy.get('channel_hash'),
                data_copy.get('next_hop'),
                data_copy.get('relay_node'),
                data_copy.get('packet_data'),
                data_copy.get('signal_level_dbm'),
                data_copy.get('full_packet_len'),
                data_copy.get('additional_field3'),
                data_copy.get('additional_field4')
            )

            cur.execute(query, parameters)
            result = cur.fetchone()
            conn.commit()

            cur.close()
            conn.close()

            return jsonify({
                'status': 'success',
                'message': 'Data added successfully',
                'line_num': result[0],
                'created_at': result[1].isoformat()
            })

    except Exception as e:
        return jsonify({'status': 'error', 'message': str(e)}), 500
    
@app.route('/api/health', methods=['GET'])
def health_check():
    """Проверка здоровья API и подключения к БД"""
    try:
        conn = get_db_connection()
        cur = conn.cursor()
        cur.execute("SELECT 1")
        cur.close()
        conn.close()
        
        return jsonify({
            'status': 'healthy',
            'database': 'connected',
            'timestamp': datetime.now().isoformat()
        })
    except Exception as e:
        return jsonify({
            'status': 'unhealthy', 
            'database': 'disconnected',
            'error': str(e)
        }), 500

@app.route('/api/lora/clear', methods=['GET', 'POST', 'DELETE'])
def clear_table():
    """Очистить всю таблицу lora_tab (GET, POST или DELETE)"""
    try:
        # Проверки подтверждения
        if request.method == 'GET':
            confirm = request.args.get('confirm', '')
            if confirm != 'yes':
                return jsonify({
                    'status': 'warning',
                    'message': 'For GET requests, add ?confirm=yes to clear table'
                }), 400
        
        elif request.method == 'POST':
            data = request.json or {}
            if data.get('confirmation') != 'YES_CLEAR_ALL':
                return jsonify({
                    'status': 'warning',
                    'message': 'For POST requests, send {"confirmation": "YES_CLEAR_ALL"}'
                }), 400
        
        conn = get_db_connection()
        cur = conn.cursor()
        
        # Получаем количество записей перед очисткой
        cur.execute("SELECT COUNT(*) FROM lora_tab")
        records_count = cur.fetchone()[0]
        
        # ПРОСТО ОЧИЩАЕМ ТАБЛИЦУ БЕЗ СБРОСА ПОСЛЕДОВАТЕЛЬНОСТИ
        cur.execute("DELETE FROM lora_tab")
        
        conn.commit()
        cur.close()
        conn.close()
        
        return jsonify({
            'status': 'success', 
            'message': f'Table cleared successfully. Removed {records_count} records.',
            'records_removed': records_count
        })
        
    except Exception as e:
        return jsonify({'status': 'error', 'message': str(e)}), 500
    
@app.route('/api/lora/view', methods=['GET'])
def view_table_html():
    """HTML представление таблицы"""
    try:
        conn = get_db_connection()
        cur = conn.cursor(cursor_factory=RealDictCursor)
        cur.execute("SELECT * FROM lora_tab ORDER BY created_at DESC")
        data = cur.fetchall()
        cur.close()
        conn.close()
        
        html_template = """
        <!DOCTYPE html>
        <html>
        <head>
            <title>LoRa Data Table</title>
            <style>
                table { border-collapse: collapse; width: 100%; }
                th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
                th { background-color: #f2f2f2; }
                tr:nth-child(even) { background-color: #f9f9f9; }
            </style>
        </head>
        <body>
            <h1>LoRa Data Table ({{ count }} records)</h1>
            <table>
                <tr>
                    {% for key in keys %}
                    <th>{{ key }}</th>
                    {% endfor %}
                </tr>
                {% for row in data %}
                <tr>
                    {% for key in keys %}
                    <td>{{ row[key] }}</td>
                    {% endfor %}
                </tr>
                {% endfor %}
            </table>
        </body>
        </html>
        """
        
        keys = list(data[0].keys()) if data else []
        return render_template_string(html_template, data=data, keys=keys, count=len(data))
        
    except Exception as e:
        return f"Error: {e}", 500

if __name__ == '__main__':
    print("Starting LoRa API server...")
    print(f"Database: {DB_CONFIG['dbname']}@{DB_CONFIG['host']}:{DB_CONFIG['port']}")
    print(f"Server: http://{HOST}:{PORT}")
    print("\nAPI endpoints:")
    print("  GET    /api/lora          - Get all data")
    print("  GET    /api/lora/<id>     - Get single record") 
    print("  POST   /api/lora          - Add new record")
    print("  GET    /api/health        - Health check")
    print("  GET    /api/lora/view     - HTML view")
    print("  GET/POST/DELETE /api/lora/clear - Clear table")
    
    app.run(host='0.0.0.0', port=5001, debug=False)  # debug=False для продакшена
