#define XML_BUFFER_LEN 4096

void writeData(char *str);

extern char xml_buffer[];

#define XmlPrintf(format, args...) \
  snprintf(xml_buffer, XML_BUFFER_LEN, format, ##args); \
  writeData(xml_buffer)

#define SendErrorMessage(channel, format, args...) \
  snprintf(xml_buffer, XML_BUFFER_LEN, "<Error message=\"" format "\"/>\n", ##args); \
  writeDataSingleChannel(xml_buffer, channel)

