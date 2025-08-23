#!/usr/bin/env python3
"""
Optimized Air Mouse WebSocket Server
Real-time joystick-like mouse control
"""

import asyncio
import websockets
import pyautogui
import json
import socket
from datetime import datetime

# Configuration
PORT = 8888
MOUSE_ACCELERATION = 1.2  # Optimized for pointer movement
SMOOTH_MOVEMENT = True

# PyAutoGUI settings
pyautogui.FAILSAFE = True
pyautogui.PAUSE = 0.001

# Statistics
stats = {'commands': 0, 'movements': 0, 'clicks': 0}

def get_local_ip():
    """Get local IP address"""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.connect(("8.8.8.8", 80))
            return s.getsockname()[0]
    except:
        return "127.0.0.1"

async def handle_client(websocket):
    """Handle WebSocket client connection"""
    client_ip = websocket.remote_address[0]
    print(f"🔌 Client connected: {client_ip}")
    
    try:
        # Send welcome message
        await websocket.send(json.dumps({"type": "welcome", "message": "Air Mouse Server Ready"}))
        
        async for message in websocket:
            await process_message(message, websocket)
            
    except websockets.exceptions.ConnectionClosed:
        print(f"🔌 Client disconnected: {client_ip}")
    except Exception as e:
        print(f"❌ Client error ({client_ip}): {e}")

async def process_message(message, websocket):
    """Process incoming WebSocket message"""
    try:
        data = json.loads(message)
        msg_type = data.get('type', '')
        
        stats['commands'] += 1
        
        if msg_type == 'move':
            # Pointer-style movement (tracks device movement in air)
            delta_x = int(data.get('deltaX', 0) * MOUSE_ACCELERATION)
            delta_y = int(data.get('deltaY', 0) * MOUSE_ACCELERATION)
            
            if delta_x != 0 or delta_y != 0:
                current_x, current_y = pyautogui.position()
                screen_width, screen_height = pyautogui.size()
                
                new_x = max(0, min(screen_width - 1, current_x + delta_x))
                new_y = max(0, min(screen_height - 1, current_y + delta_y))
                
                if SMOOTH_MOVEMENT:
                    pyautogui.moveTo(new_x, new_y, duration=0.002)
                else:
                    pyautogui.moveTo(new_x, new_y)
                
                stats['movements'] += 1
                
        elif msg_type == 'command':
            command = data.get('command', '')
            
            if command == 'leftclick':
                pyautogui.click()
                stats['clicks'] += 1
                print("🖱️ Left click")
                await websocket.send(json.dumps({"type": "response", "message": "Click executed"}))
                
            elif command == 'rightclick':
                pyautogui.rightClick()
                stats['clicks'] += 1
                print("🖱️ Right click")
                
            elif command == 'doubleclick':
                pyautogui.doubleClick()
                stats['clicks'] += 1
                print("🖱️ Double click")
                
            elif command == 'scroll_up':
                pyautogui.scroll(3)
                print("⬆️ Scroll up")
                
            elif command == 'scroll_down':
                pyautogui.scroll(-3)
                print("⬇️ Scroll down")
                
        elif msg_type == 'status':
            device = data.get('device', 'Unknown')
            print(f"📱 Device connected: {device}")
            
    except json.JSONDecodeError:
        print("❌ Invalid JSON received")
    except Exception as e:
        print(f"❌ Processing error: {e}")

def print_header():
    """Print application header"""
    print("=" * 50)
    print("    🖱️ AIR MOUSE WEBSOCKET SERVER")
    print("    Optimized Joystick-like Control")
    print("=" * 50)

def print_setup_info(local_ip):
    """Print setup information"""
    print(f"📡 Server: {local_ip}:{PORT}")
    print(f"🎯 Mouse Acceleration: {MOUSE_ACCELERATION}x")
    print(f"🌊 Smooth Movement: {'✅' if SMOOTH_MOVEMENT else '❌'}")
    print()
    
    print("🔧 ESP8266 SETUP:")
    print(f'   const char* serverIP = "{local_ip}";')
    print(f"   const int serverPort = {PORT};")
    print()
    
    print("🎮 POINTER CONTROLS:")
    print("   Move NodeMCU in air → Cursor follows movement")
    print("   Keep device steady → Cursor stops")
    print("   Short press → Toggle ON/OFF")
    print("   Long press  → Calibrate center")
    print("   Very long   → Left click")
    print()

async def stats_monitor():
    """Monitor and display statistics"""
    while True:
        await asyncio.sleep(30)
        if stats['commands'] > 0:
            print(f"📊 Stats: {stats['commands']} cmds, "
                  f"{stats['movements']} moves, {stats['clicks']} clicks")

async def main():
    """Main application entry point"""
    print_header()
    
    # Get local IP
    local_ip = get_local_ip()
    print_setup_info(local_ip)
    
    print("🚀 Starting WebSocket server...")
    print("🛑 Press Ctrl+C to stop")
    print("-" * 50)
    
    try:
        # Start WebSocket server
        server = await websockets.serve(
            handle_client, 
            "0.0.0.0", 
            PORT,
            ping_interval=20,
            ping_timeout=10
        )
        
        print(f"✅ Server running on {local_ip}:{PORT}")
        
        # Start statistics monitor
        stats_task = asyncio.create_task(stats_monitor())
        
        # Keep server running
        await server.wait_closed()
        
    except KeyboardInterrupt:
        print("\n🛑 Server stopped")
        print(f"📊 Final: {stats['commands']} commands, "
              f"{stats['movements']} movements, {stats['clicks']} clicks")
        
    except OSError as e:
        if "Address already in use" in str(e):
            print(f"\n❌ Port {PORT} in use! Try a different port.")
        else:
            print(f"\n❌ Network error: {e}")
            
    except Exception as e:
        print(f"\n❌ Error: {e}")

def check_dependencies():
    """Check required dependencies"""
    missing = []
    
    try:
        import pyautogui
        import websockets
    except ImportError as e:
        if 'pyautogui' in str(e):
            missing.append('pyautogui')
        if 'websockets' in str(e):
            missing.append('websockets')
    
    if missing:
        print("❌ Missing dependencies:")
        for dep in missing:
            print(f"   pip install {dep}")
        return False
    return True

if __name__ == "__main__":
    if not check_dependencies():
        exit(1)
    
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n👋 Goodbye!")