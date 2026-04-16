#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlsave.h>  

// Yolcu Şablonu
typedef struct {
    char ticket_id[10];       
    char timestamp[25];       
    float baggage_weight;     
    int loyalty_points;       
    char status[16];          
    char destination[35];     
    char cabin_class[15];     
    int seat_num;             
    char app_ver[15];         
    char passenger_name[100]; 
} Passenger;

// --- ADIM 1: CSV'DEN BINARY'YE ---
void csv_to_binary(const char *csv_path, const char *bin_path, int separator, int opsys) {
    FILE *csv = fopen(csv_path, "r");
    FILE *bin = fopen(bin_path, "wb");
    
    if (!csv || !bin) {
        perror("Dosya acma hatasi");
        return;
    }

    // 1. Ayiriciyi (Delimiter) Belirle [cite: 82]
    char delim[2];
    if (separator == 2) strcpy(delim, "\t");
    else if (separator == 3) strcpy(delim, ";");
    else strcpy(delim, ",");

    char line[1024];
    // Baslik satirini oku ve atla
    if (!fgets(line, sizeof(line), csv)) return;

    while (fgets(line, sizeof(line), csv)) {
        // 2. Satir Sonu Temizligi (Opsys) [cite: 14, 84]
        // Windows (\r\n), Linux (\n) ve Mac (\n) uyumu icin \r ve \n karakterlerini sileriz
        line[strcspn(line, "\r\n")] = 0;

        Passenger p;
        char *ptr = line;
        char *token;

        // 3. Dinamik Parçalama
        // Her sütun için 'delim' degiskenini kullanarak ilerliyoruz
        token = strsep(&ptr, delim); if(token) strcpy(p.ticket_id, token);
        token = strsep(&ptr, delim); if(token) strcpy(p.timestamp, token);
        token = strsep(&ptr, delim); p.baggage_weight = (token) ? atof(token) : 0.0f;
        token = strsep(&ptr, delim); p.loyalty_points = (token) ? atoi(token) : 0;
        token = strsep(&ptr, delim); if(token) strcpy(p.status, token);
        token = strsep(&ptr, delim); if(token) strcpy(p.destination, token);
        token = strsep(&ptr, delim); if(token) strcpy(p.cabin_class, token);
        token = strsep(&ptr, delim); p.seat_num = (token) ? atoi(token) : 0;
        token = strsep(&ptr, delim); if(token) strcpy(p.app_ver, token);
        token = strsep(&ptr, delim); if(token) strcpy(p.passenger_name, token);

        // Struct'ı binary olarak dosyaya yaz 
        fwrite(&p, sizeof(Passenger), 1, bin);
    }

    fclose(csv);
    fclose(bin);
    printf("Dönüsüm Tamamlandi: %s -> %s (Ayirici: '%s')\n", csv_path, bin_path, delim);
}
// --- ADIM 2: BINARY'DEN XML'E ---
void binary_to_xml(const char *bin_path, const char *xml_path) {
    FILE *bin = fopen(bin_path, "rb");
    if (!bin) return;

    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST "flightlogs");
    xmlDocSetRootElement(doc, root_node);

    Passenger p;
    int entry_id = 1;

    while (fread(&p, sizeof(Passenger), 1, bin) == 1) {
        char id_str[20];
        sprintf(id_str, "%d", entry_id);
        
        xmlNodePtr entry_node = xmlNewChild(root_node, NULL, BAD_CAST "entry", NULL);
        xmlNewProp(entry_node, BAD_CAST "id", BAD_CAST id_str);

        xmlNodePtr ticket_node = xmlNewChild(entry_node, NULL, BAD_CAST "ticket", NULL);
        xmlNewChild(ticket_node, NULL, BAD_CAST "ticket_id", BAD_CAST p.ticket_id);
        xmlNewChild(ticket_node, NULL, BAD_CAST "destination", BAD_CAST p.destination);
        if (strlen(p.app_ver) > 0) xmlNewChild(ticket_node, NULL, BAD_CAST "app_ver", BAD_CAST p.app_ver);

        xmlNodePtr metrics_node = xmlNewChild(entry_node, NULL, BAD_CAST "metrics", NULL);
        xmlNewProp(metrics_node, BAD_CAST "status", BAD_CAST p.status);
        xmlNewProp(metrics_node, BAD_CAST "cabin_class", BAD_CAST p.cabin_class);

        char temp[20];
        sprintf(temp, "%.1f", p.baggage_weight); xmlNewChild(metrics_node, NULL, BAD_CAST "baggage_weight", BAD_CAST temp);
        sprintf(temp, "%d", p.loyalty_points); xmlNewChild(metrics_node, NULL, BAD_CAST "loyalty_points", BAD_CAST temp);
        sprintf(temp, "%d", p.seat_num); xmlNewChild(metrics_node, NULL, BAD_CAST "seat_num", BAD_CAST temp);

        if (strlen(p.timestamp) > 0) xmlNewChild(entry_node, NULL, BAD_CAST "timestamp", BAD_CAST p.timestamp);

        // Hex Kodu Hesaplama
        char hex_str[20] = "";
        unsigned char *c = (unsigned char *)p.passenger_name;
        int bytes = 1;
        if ((c[0] & 0xE0) == 0xC0) bytes = 2;
        else if ((c[0] & 0xF0) == 0xE0) bytes = 3;
        else if ((c[0] & 0xF8) == 0xF0) bytes = 4;

        for(int i = 0; i < bytes; i++) sprintf(hex_str + (i * 2), "%02X", c[i]);

        xmlNodePtr pass_node = xmlNewChild(entry_node, NULL, BAD_CAST "passenger_name", BAD_CAST p.passenger_name);
        xmlNewProp(pass_node, BAD_CAST "current_encoding", BAD_CAST "UTF-8");
        xmlNewProp(pass_node, BAD_CAST "first_char_hex", BAD_CAST hex_str);

        entry_id++;
    }
    
    fclose(bin);
    xmlSaveFormatFileEnc(xml_path, doc, "UTF-8", 1);
    xmlFreeDoc(doc);
    xmlCleanupParser();
    printf("Adim 2 Tamam: Binary -> XML donusumu yapildi.\n");
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/xmlschemas.h>

// Yardim menusu (Ödevdeki -h bayragı icin)
void print_usage() {
    printf("\n--- FLIGHT PASSENGER LOG CONVERSION TOOL ---\n");
    printf("Usage:\n");
    printf("  ./flightTool <input_file> <output_file> <conversion_type> -separator <1|2|3> -opsys <1|2|3> [-encoding <1|2|3>] [-h]\n\n");
    printf("Arguments:\n");
    printf("  <conversion_type>: 1: CSV->Bin, 2: Bin->XML, 3: XSD Valid, 4: XML Encoding Switch [cite: 76, 80]\n");
    printf("  -separator: 1: Comma, 2: Tab, 3: Semicolon [cite: 82]\n");
    printf("  -opsys:     1: Windows, 2: Linux, 3: MacOS [cite: 84]\n");
    printf("  -encoding:  1: UTF-16LE, 2: UTF-16BE, 3: UTF-8 (Auto-detect) [cite: 89, 91]\n");
}

int validate_xml(const char *xml_filename, const char *xsd_filename) {
    xmlDocPtr doc;
    xmlSchemaPtr schema = NULL;
    xmlSchemaParserCtxtPtr ctxt;
    int ret = -1;

    xmlLineNumbersDefault(1);
    
    // XSD dosyasını oku ve parse et
    ctxt = xmlSchemaNewParserCtxt(xsd_filename);
    schema = xmlSchemaParse(ctxt);
    xmlSchemaFreeParserCtxt(ctxt);

    // XML dosyasını oku
    doc = xmlReadFile(xml_filename, NULL, 0);
    if (doc == NULL) {
        fprintf(stderr, "Hata: %s dosyası okunamadı.\n", xml_filename);
    } else {
        xmlSchemaValidCtxtPtr vctxt = xmlSchemaNewValidCtxt(schema);
        ret = xmlSchemaValidateDoc(vctxt, doc); // 0 ise başarılı
        
        if (ret == 0) printf("%s başarıyla doğrulandı (Validated).\n", xml_filename);
        else if (ret > 0) printf("%s doğrulama hatası (Validation Failed).\n", xml_filename);
        else printf("%s doğrulama sırasında iç hata oluştu.\n", xml_filename);

        xmlSchemaFreeValidCtxt(vctxt);
        xmlFreeDoc(doc);
    }

    if (schema != NULL) xmlSchemaFree(schema);
    xmlSchemaCleanupTypes();
    
    return ret;
}

void convert_xml_encoding(const char *input_xml, const char *output_xml, int encoding_type) {
    // XML dosyasını belleğe yükle
    xmlDocPtr doc = xmlReadFile(input_xml, NULL, 0);
    if (doc == NULL) {
        fprintf(stderr, "Hata: Kaynak XML dosyasi okunamadi: %s\n", input_xml);
        return;
    }

    const char *encoding_str;
    const char *hex_val;
    
    // 1. Hedef encoding ve örnek hex değerlerini belirle
    if (encoding_type == 1) { 
        encoding_str = "UTF-16LE"; 
        hex_val = "D600"; // Örnek: 'Ö' harfinin UTF-16LE karşılığı
    } else if (encoding_type == 2) { 
        encoding_str = "UTF-16BE"; 
        hex_val = "00D6"; // Örnek: 'Ö' harfinin UTF-16BE karşılığı
    } else { 
        encoding_str = "UTF-8"; 
        hex_val = "C396"; // Örnek: 'Ö' harfinin UTF-8 karşılığı
    }

    // 2. XML Ağacını tara ve passenger_name niteliklerini güncelle
    xmlNodePtr root = xmlDocGetRootElement(doc);
    for (xmlNodePtr entry = root->children; entry; entry = entry->next) {
        if (entry->type == XML_ELEMENT_NODE && strcmp((const char*)entry->name, "entry") == 0) {
            for (xmlNodePtr child = entry->children; child; child = child->next) {
                if (strcmp((const char*)child->name, "passenger_name") == 0) {
                    // current_encoding niteliğini yeni değere set et
                    xmlSetProp(child, BAD_CAST "current_encoding", BAD_CAST encoding_str);
                    // first_char_hex niteliğini yeni endianness/encodinge göre güncelle
                    xmlSetProp(child, BAD_CAST "first_char_hex", BAD_CAST hex_val);
                }
            }
        }
    }

    // 3. Dosyayı yeni encoding ile kaydet (BOM otomatik eklenir)
    int bytes_saved = xmlSaveFormatFileEnc(output_xml, doc, encoding_str, 1);

    if (bytes_saved >= 0) {
        printf("Mod 4 Tamam: %s -> %s donusturuldu (Hedef: %s)\n", input_xml, output_xml, encoding_str);
    } else {
        printf("Hata: Encoding donusumu sirasinda bir sorun olustu.\n");
    }

    xmlFreeDoc(doc);
}

int main(int argc, char *argv[]) {
    // 1. Temel Argüman Sayısı Kontrolü
    if (argc < 4) {
        print_usage();
        return 1;
    }

    // 2. Zorunlu Konumsal Argümanlar
    char *input_file = argv[1];
    char *output_file = argv[2];
    int conversion_type = atoi(argv[3]);

    // 3. Varsayılan Değerler ve Opsiyonel Argümanlar
    int separator = 1; // Default comma [cite: 7]
    int opsys = 2;     // Default windows [cite: 7]
    int encoding = 3;  // Default UTF-8 [cite: 18]

    // 4. Bayrakları (Flags) Tara
    for (int i = 4; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            print_usage();
            return 0;
        } else if (strcmp(argv[i], "-separator") == 0 && i + 1 < argc) {
            separator = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-opsys") == 0 && i + 1 < argc) {
            opsys = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-encoding") == 0 && i + 1 < argc) {
            encoding = atoi(argv[++i]);
        }
    }

    // 5. Okunan Değerlerin Doğrulanması (Debug)
    printf("\n--- Gelen Parametreler ---\n");
    printf("Input File      : %s\n", input_file);
    printf("Output File     : %s\n", output_file);
    printf("Conversion Type : %d\n", conversion_type);
    printf("Separator       : %d (%s)\n", separator, (separator == 1 ? "Comma" : (separator == 2 ? "Tab" : "Semicolon")));
    printf("Operating System: %d (%s)\n", opsys, (opsys == 1 ? "Windows" : (opsys == 2 ? "Linux" : "MacOS")));
    if (conversion_type == 4) {
        printf("Target Encoding : %d\n", encoding);
    }
    printf("--------------------------\n");
    // --- FONKSİYON DALLANMASI ---
    switch (conversion_type) {
        case 1:
            // CSV -> Binary Dönüşümü
            // Fonksiyonu artık yeni parametreleri (separator, opsys) ile çağırıyoruz
            csv_to_binary(input_file, output_file, separator, opsys);
            break;

        case 2:
            // Binary -> XML (UTF-8) Dönüşümü
            binary_to_xml(input_file, output_file);
            break;

        case 3:
            printf("Mod 3: XML Doğrulaması Başlatılıyor...\n");
             // Ödev isterine göre: argv[1] XML, argv[2] XSD dosyasıdır
             validate_xml(input_file, output_file); 
            break;

        case 4:
            printf("Mod 4: XML Encoding Donusturucu Baslatiliyor...\n");
            convert_xml_encoding(input_file, output_file, encoding);
            break;

        default:
            printf("Hata: Gecersiz conversion_type (%d)!\n", conversion_type);
            print_usage();
            return 1;
    }

    // Gelecek Adim: Burada switch(conversion_type) ile fonksiyonlara dallanacagiz.

    return 0;
}
