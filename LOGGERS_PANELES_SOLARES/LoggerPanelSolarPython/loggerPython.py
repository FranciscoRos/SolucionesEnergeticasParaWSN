import serial
import datetime
import time

# --- Configuración ---
SERIAL_PORT = 'COM6'  # Reemplaza con tu puerto COM (ej. '/dev/ttyUSB0' en Linux)
BAUD_RATE = 115200
OUTPUT_FILE = 'registro_solar.csv'
# ---------------------

print(f"Conectando a {SERIAL_PORT}...")

try:
    # Abrir conexión serial
    with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=5) as ser:
        print(f"Conectado. Registrando datos en {OUTPUT_FILE}...")
        
        # Abrir archivo CSV para escribir
        with open(OUTPUT_FILE, 'a', encoding='utf-8') as f:
            
            # Leer la primera línea (encabezado) del Arduino
            try:
                header = ser.readline().decode('utf-8').strip()
                # Escribimos nuestro propio encabezado con Timestamp real
                f.write(f"Timestamp_PC,{header}\n")
                print(f"Encabezado recibido: {header}")
            except Exception as e:
                print(f"No se pudo leer el encabezado: {e}")

            # Bucle principal para leer datos
            while True:
                try:
                    # Leer una línea de datos del Arduino
                    linea_datos = ser.readline().decode('utf-8').strip()
                    
                    if linea_datos:
                        # Obtener el timestamp actual
                        timestamp_pc = datetime.datetime.now().isoformat()
                        
                        # Combinar timestamp y datos
                        registro_completo = f"{timestamp_pc},{linea_datos}"
                        
                        # Imprimir en consola y guardar en archivo
                        print(registro_completo)
                        f.write(f"{registro_completo}\n")
                        f.flush() # Forzar escritura al disco

                except serial.SerialException as se:
                    print(f"Error de serial: {se}")
                    break
                except UnicodeDecodeError:
                    print("Error de decodificación. Omitiendo línea.")
                except KeyboardInterrupt:
                    print("\nRegistro detenido por el usuario.")
                    break

except serial.SerialException as e:
    print(f"Error: No se pudo abrir el puerto {SERIAL_PORT}. Verifica la conexión.")
    print(e)

print("Proceso finalizado.")