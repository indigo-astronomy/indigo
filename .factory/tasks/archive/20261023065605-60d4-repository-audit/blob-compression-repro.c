// Audit-only reproducer for the HTTP BLOB caller's input-sized gzip buffer.
#include <stdio.h>
#include <zlib.h>
#include <indigo/indigo_uni_io.h>
int main(void) {
 char input[32];
 unsigned char output[32];
 for (int i = 0; i < 32; i++) { input[i] = (char)i; }
 unsigned size = sizeof(output);
 indigo_uni_compress("image", input, sizeof(input), output, &size);
 printf("HTTP caller buffer: %zu bytes; gzip output size: %u\n", sizeof(output), size);
 unsigned char recovered[128];
 z_stream stream = { 0 };
 inflateInit2(&stream, 31);
 stream.next_in = output; stream.avail_in = size;
 stream.next_out = recovered; stream.avail_out = sizeof(recovered);
 int result = inflate(&stream, Z_FINISH);
 printf("gzip decoder status: %d (expected Z_STREAM_END = %d), recovered: %lu/%zu\n", result, Z_STREAM_END, stream.total_out, sizeof(input));
 inflateEnd(&stream);
 return result == Z_STREAM_END ? 0 : 1;
}
