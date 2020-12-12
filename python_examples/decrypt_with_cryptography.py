import binascii
from binascii import unhexlify
from cryptography.hazmat.primitives.ciphers.aead import AESGCM

"""
Example Script to convert and decrypt message from im350 smart meter.
Usage: Just enter enter the message from you smart meter and your decryption key.
"""
# Hex String, 246 Chars // 123 Bytes, Starting/Stopping with 7E
message_hex_string = '<Your Message>'
# Hex String 32 Chars // 16 Bytes, you get the key from you provider!
decryption_key_hex_string = '<Your Decryption Key>'


# region helperfunctions
def decrypt(system_title, frame_counter, encryption_key, aad, apdu, iv=None):
    aesgcm = AESGCM(unhexlify(encryption_key))

    if not iv:
        iv = create_iv(frame_counter, system_title)

    apdu = aesgcm.encrypt(iv, apdu, aad)
    apdu = apdu

    return apdu


def create_iv(frame_counter, system_title):
    nonce = bytearray(11)
    nonce[0:7] = system_title
    nonce[8] = ((frame_counter >> 24) & 0xFF)
    nonce[9] = ((frame_counter >> 16) & 0xFF)
    nonce[10] = ((frame_counter >> 8) & 0xFF)
    nonce[11] = (frame_counter & 0xFF)
    return nonce
#endregion


"""
Total Message length = 123 Byte
Startbyte = 7e
Endbyte = 7e
                          Size        Start   End
Frame/Invocation Counter  4   Bytes   26      30
HDLC Frame                106 Bytes   14      120
Encrypted Ciphertext      90  Bytes   30      120
Serial No.                5   Bytes   16      21
Manufacturer              3   Bytes   21      24
"""


message_bytes = binascii.unhexlify(message_hex_string)
hdlc_frame = message_bytes[14:120]
ciphertext_encrypted = message_bytes[30:120]
manufacturer = message_bytes[16:21]
serial_number = message_bytes[21:24]
system_title = manufacturer + serial_number
frame_counter = message_bytes[26:30]
iv = system_title + frame_counter
aad_hex = binascii.unhexlify('d0d1d2d3d4d5d6d7d8d9dadbdcdddedf')


ciphertext_decrypted = decrypt(iv=iv, system_title=system_title, frame_counter=frame_counter,
                               encryption_key=decryption_key_hex_string, aad=aad_hex, apdu=ciphertext_encrypted)

ciphertext_decrypted_hex = ciphertext_decrypted.hex()

counter_reading_p_in = int.from_bytes(ciphertext_decrypted[57:61], byteorder="big")
counter_reading_p_out = int.from_bytes(ciphertext_decrypted[62:66], byteorder="big")
counter_reading_q_in = int.from_bytes(ciphertext_decrypted[67:71], byteorder="big")
counter_reading_q_out = int.from_bytes(ciphertext_decrypted[72:76], byteorder="big")
current_power_usage_in = int.from_bytes(ciphertext_decrypted[77:81], byteorder="big")
current_power_usage_out = int.from_bytes(ciphertext_decrypted[82:86], byteorder="big")

message_year = (ciphertext_decrypted[6] << 8) + (ciphertext_decrypted[7]);
message_month = ciphertext_decrypted[8];
message_day = ciphertext_decrypted[9];
message_hour = ciphertext_decrypted[11];
message_minute = ciphertext_decrypted[12];
message_second = ciphertext_decrypted[13];
serial_number_int = int.from_bytes(serial_number, byteorder="big")
message_date_string = f'{message_year}-{message_month}-{message_day} {message_hour}:{message_minute}:{message_second}'

print(f'Serial number: {serial_number_int}')
print(f'Message Date: {message_date_string}')
print(f'Counter Reading P IN: {counter_reading_p_in} Wh')
print(f'Counter Reading P OUT: {counter_reading_p_out} Wh')
print(f'Counter Reading Q IN: {counter_reading_q_in} varh')
print(f'Counter Reading Q OUT: {counter_reading_q_out} varh')
print(f'Current Power Usage IN: {counter_reading_p_in} W')
print(f'Current Power Usage OUT: {counter_reading_p_in} W')