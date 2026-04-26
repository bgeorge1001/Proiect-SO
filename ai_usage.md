# Utilizarea asistentului AI la cele 2 functii : parse_condition si match_condition

    *Tool folosit*: Gemini Pro 3.1

    ## Prompt
    You are an expert programmer in C and operating systems. I have to do a C project called city_manager. The Report structure is a fixed-size binary record containing a report_id identifier (int), inspector_name (char array), latitude and longitude coordinates (float), category (char array), severity level (int), timestamp (time_t) and problem details in description (char array). Now, I want you to write the following fuctions for me : int parse_condition(const char *input, char *field, char *op, char *value); which splits a field:operator:value string into its three parts,
    int match_condition(Report *r, const char *field, const char *op, const char *value);which returns 1 if the record satisfies the condition and 0 otherwise.  The filter command is called as --filter <district> <condition...> and each condition has format field:operator:value. Supported fields are severity, category, inspector, timestamp. Supported operators are ==, !=, <, <=, >, >=.
    
    ## Ce a generat AI-ul
    AI-ul mi-a generat cele 2 functii, care au fost bune. Acesta mi-a generat si macrocomenzi.

    ## Ce am modificat eu
    Am eliminat macrocomenzile (pentru ca nu sunt familiarizat cu ele), si am inlocuit unde s-a folosit de macrocomenzi, care mi-a rezultat intr-un cod mult mai lung.

    ## Ce am invatat
    Prompt-ul este 'decisive' pentru AI si conteaza foarte mult pentru a genera un cod de calitate, oferand multe si precise detalii.

    ## Concluzie 
    In final, prompt-ul in care ii spui AI-ului "You are a ..." mi se pare benefic. Cred ca face diferenta si il voi folosi de acum inainte. Pe langa asta, detaliile si contextul oferit sunt cele mai importante, cand vine vorba de prompt-uri pt AI.
