#include "/Users/polakovic/Development/indigo/indigo_drivers/agent_mcp/indigo_agent_mcp.c"
int main(void) {
 uint32_t size;
 while (fread(&size, sizeof(size), 1, stdin) == 1) {
  if (size > 65536) return 2;
  char *text = malloc((size_t)size + 1);
  if (!text || fread(text, 1, size, stdin) != size) return 3;
  text[size] = 0;
  json_document doc = { .text = text };
  jsmn_parser p; jsmn_init(&p);
  doc.count = valid_utf8((const unsigned char *)text, size) ? jsmn_parse(&p, text, size, doc.tokens, MCP_TOKEN_LIMIT) : -1;
  int next = 0;
  unsigned char good = doc.count > 0 && token_begin(&doc, 0) == skip_space(text, 0, size) && valid_value(&doc, 0, 0, &next) && next == doc.count && skip_space(text, token_end(&doc, 0), size) == size;
  if (fwrite(&good, 1, 1, stdout) != 1) return 4;
  free(text);
 }
 return 0;
}
