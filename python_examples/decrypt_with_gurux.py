import binascii
from gurux_dlms import *
from xml.dom.minidom import parseString, Node
import re

"""
Example Script to convert and decrypt message from im350 smart meter with gurux python library.
Usage: Just enter enter the message from you smart meter and your decryption key.
"""

# Hex String, 246 Chars // 123 Bytes, Starting/Stopping with 7E
message_hex_string = '<Your Message>'
# Hex String 32 Chars // 16 Bytes, you get the key from you provider!
decryption_key_hex_string = '<Your Decryption Key>'

decryption_key = binascii.unhexlify(decryption_key_hex_string)
message_bytes = binascii.unhexlify(message_hex_string)

translator = GXDLMSTranslator()
translator.comments = True
translator.blockCipherKey = decryption_key

message_ = translator.messageToXml(message_bytes)
message = re.sub(r'(<!--Decrypt)(.*)', "", message_)

root = parseString(message)
data_notification = root.getElementsByTagName("DataNotification")
data_nodes = data_notification[0].getElementsByTagName("UInt32")

message_date_string = data_notification[0].childNodes[3].nodeValue
serial_number = re.findall('Serial number:.*', message_)[0]
counter_reading_p_in = int.from_bytes(bytes.fromhex(data_nodes[0].getAttribute('Value')), byteorder="big")
counter_reading_p_out = int.from_bytes(bytes.fromhex(data_nodes[1].getAttribute('Value')), byteorder="big")
counter_reading_q_in = int.from_bytes(bytes.fromhex(data_nodes[2].getAttribute('Value')), byteorder="big")
counter_reading_q_out = int.from_bytes(bytes.fromhex(data_nodes[3].getAttribute('Value')), byteorder="big")
current_power_usage_in = int.from_bytes(bytes.fromhex(data_nodes[4].getAttribute('Value')), byteorder="big")
current_power_usage_out = int.from_bytes(bytes.fromhex(data_nodes[5].getAttribute('Value')), byteorder="big")

print(f'{serial_number}')
print(f'Message Date: {message_date_string}')
print(f'Counter Reading P IN: {counter_reading_p_in} Wh')
print(f'Counter Reading P OUT: {counter_reading_p_out} Wh')
print(f'Counter Reading Q IN: {counter_reading_q_in} varh')
print(f'Counter Reading Q OUT: {counter_reading_q_out} varh')
print(f'Current Power Usage IN: {counter_reading_p_in} W')
print(f'Current Power Usage OUT: {counter_reading_p_in} W')