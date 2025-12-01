#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// --- BUFFER SIZE CONFIG
#define MAX_WORDS 10000        /*Number of maximum words in index table*/
#define MAX_STOP_WORDS 100     /*Number of maximum stop words*/
#define MAX_WORD_LEN 50        /*Word length*/
#define MAX_LINE_LIST_LEN 1000 /*MAximum length of line*/
#define BUFF_SIZE 1024

#define TRUE 1
#define FALSE 0

// --- DATA STRUCTURES

// Entry
typedef struct
{
    char word[MAX_WORD_LEN];       // Payload
    int count;                     // Number of appearance
    char lines[MAX_LINE_LIST_LEN]; // lines
} WordIndex;

// Global buffers
char stop_words[MAX_STOP_WORDS][MAX_WORD_LEN]; // Stop words array
int stop_word_count = 0;

WordIndex index_table[MAX_WORDS]; // Index table
int index_count = 0;              // number of words in index table

// --- PROTOTYPES

// Module 1: Initialization
void load_stop_words(const char *filename);

// Module 2: Filtering & sanitization
void to_lowercase(char *str);
int is_stop_word(const char *word);
int is_proper_noun(const char *word, int is_start_of_sentence);

// Module 3: Core indexing Logic
// This function will decide whether to add or update index table
void process_word(char *raw_word, int line_num, int is_start_sentence);

// Module 4: Utilities
// Sort index table alphabetically order
void sort_index_table();

// Append line into lines array
void append_line_number(int index, int line_num);

// --- MAIN ENTRY
int main(int argc, char *argv[])
{

    char *filename = (argv[1] != NULL) ? argv[1] : "alice30.txt";

    // Step 1. Load black list
    load_stop_words("stopw.txt");

    // Step 2. Open file
    FILE *file = fopen(filename, "r");

    if (file == NULL)
    {
        printf("[ERROR] Can NOT open file: %s\r\n", filename);
        return 1;
    }

    char line_buffer[BUFF_SIZE];
    int line_number = 0;

    // Flag : to see if we are at the begining of sentences or not
    //  1 = True, 0 = False
    int is_start_of_sentence = TRUE;

    // Step 3. Parsing loop
    while (fgets(line_buffer, sizeof(line_buffer), file) != NULL)
    {
        line_number++;

        char current_word[MAX_WORD_LEN];
        int len = strlen(line_buffer);

        for (int i = 0; i <= len; i++)
        {
            char c = line_buffer[i];

            // Tokenization
            if (isalpha(c))
            {
                // Append char into current word
                int cur_len = strlen(current_word);
                if (cur_len < MAX_WORD_LEN - 1)
                {
                    current_word[cur_len] = c;
                    current_word[cur_len + 1] = '\0';
                }
            }
            else
            {
                // Receive delimeter ' '
                if (strlen(current_word) > 0)
                {
                    // Send word into processing pipeline
                    process_word(current_word,
                                 line_number,
                                 is_start_of_sentence);

                    // Reset words buffer
                    current_word[0] = '\0';

                    // After processing 1 word, we won't be at the begining of sentence
                    is_start_of_sentence = 0;
                }

                // Check if after the delimerter is there end of senetence
                if (c == '.' || c == '?' || c == '!')
                {
                    is_start_of_sentence = 1;
                }
            }
        }
    }
    fclose(file);

    // Step 4. Sort and export
    sort_index_table();

    printf("\n--- INDEX TABLE ---\n");
    for (int i = 0; i < index_count; i++)
    {
        printf("%-15s %d, %s\n",
               index_table[i].word,
               index_table[i].count,
               index_table[i].lines);
    }
    return 0;
}

// --- IMPLEMENTATION
void load_stop_words(const char *filename)
{
    FILE *fp = fopen(filename, "r");

    if (fp == NULL)
    {
        printf("[ERROR] Can NOT open stopw files: %s\r\n", filename);
        return;
    }

    char line_buffer[BUFF_SIZE];
    while (fgets(line_buffer, sizeof(line_buffer), fp) != NULL)
    {
        if (stop_word_count >= MAX_STOP_WORDS)
        {
            printf("[WARNING] Stop word list full! Stopped loading at %d words. \r\n", stop_word_count);
            break;
        }

        if (sscanf(line_buffer, "%s", stop_words[stop_word_count]) == 1)
        {
            stop_word_count++;
        };
    }

    fclose(fp);
    printf("[INFO] Done reading stopw, read: %d words\r\n", stop_word_count);
    return;
}

void process_word(char *raw_word, int line_num, int is_start_sentence)
{

    // 1. Validation
    if (is_proper_noun(raw_word, is_start_sentence))
    {
        return; // drop this word
    }

    // 2. Sanitization: Into lower case
    to_lowercase(raw_word);

    // 0. Ban alice
    if (strcmp(raw_word, "alice") == 0)
    {
        return; // Alice == cook
    }
    // 3. Filtering: Check stop Words
    if (is_stop_word(raw_word))
    {
        return; // drop word
    }

    // 4. Processing: Add into index or update
    int found_index = -1;
    // Find if word is already in the table or not
    for (int i = 0; i < index_count; i++)
    {
        if (strcmp(index_table[i].word, raw_word) == 0)
        {
            found_index = i;
            break;
        }
    }

    if (found_index != -1)
    {
        // Add appearance times
        index_table[found_index].count++;

        // Add index into list
        append_line_number(found_index, line_num);
    }
    else
    {
        // TRƯỜNG HỢP 2: TỪ MỚI (CHƯA CÓ) [cite: 56]

        // Kiểm tra xem bảng đã đầy chưa để tránh lỗi
        if (index_count >= MAX_WORDS)
        {
            printf("[WARN] Index table full!\n");
            return;
        }

        // Copy từ mới vào bảng tại vị trí index_count
        strcpy(index_table[index_count].word, raw_word);

        // Khởi tạo số lần xuất hiện là 1
        index_table[index_count].count = 1;

        // Khởi tạo danh sách dòng
        sprintf(index_table[index_count].lines, "%d", line_num); // Ghi đè chuỗi mới

        // Tăng tổng số lượng từ trong bảng
        index_count++;
    }
}

int is_proper_noun(const char *word, int is_start_sentence)
{
    // Return 1 if it is noud, 0 if legit
    if (isupper(word[0]) && !is_start_sentence)
    {
        return 1;
    }
    return 0;
}

void to_lowercase(char *str)
{
    for (int i = 0; str[i] != '\0'; i++)
    {
        if (isupper(str[i]))
        {
            str[i] = tolower(str[i]);
        }
    }
}

void append_line_number(int index, int line_num)
{
    // get pointer to current string line
    char *lines_str = index_table[index].lines;

    // Check dupliacte

    int len = strlen(lines_str);
    if (len > 0)
    {
        // Tìm dấu phẩy cuối cùng hoặc khoảng trắng để lấy số cuối
        // Tuy nhiên, để đơn giản cho bài tập C cơ bản:
        // Chúng ta sẽ dùng một buffer tạm để kiểm tra xem số dòng đã tồn tại trong chuỗi chưa
        char temp_check[20];
        sprintf(temp_check, "%d", line_num);

        // Kiểm tra sơ bộ: Nếu chuỗi lines đã chứa số dòng này thì cẩn thận
        // Nhưng để chính xác tuyệt đối thì nên parse.
        // Ở mức độ bài tập này, ta chấp nhận logic:
        // Nếu dòng hiện tại > dòng cuối cùng đã lưu thì mới thêm.

        // CÁCH ĐƠN GIẢN NHẤT:
        // Kiểm tra xem chuỗi có kết thúc bằng số dòng này không.
        // Ví dụ: "1, 5" và line_num là 5 -> Không thêm.

        // Tạo chuỗi format thử: ", 5"
        char suffix_check[MAX_LINE_LIST_LEN];
        sprintf(suffix_check, ", %d", line_num);

        // Nếu lines_str kết thúc bằng suffix_check -> Đã có rồi -> Return
        int suffix_len = strlen(suffix_check);
        if (len >= suffix_len && strcmp(lines_str + len - suffix_len, suffix_check) == 0)
        {
            return;
        }
        // Trường hợp số đầu tiên (chưa có dấu phẩy), so sánh trực tiếp
        if (strcmp(lines_str, temp_check) == 0)
        {
            return;
        }

        // Nếu chưa có, nối thêm dấu phẩy và số dòng
        char buffer[20];
        sprintf(buffer, ", %d", line_num);

        // Kiểm tra tràn bộ đệm trước khi nối
        if (strlen(lines_str) + strlen(buffer) < MAX_LINE_LIST_LEN)
        {
            strcat(lines_str, buffer);
        }
    }
    else
    {
        // Nếu chuỗi rỗng (lần đầu tiên thêm), chỉ thêm số
        sprintf(lines_str, "%d", line_num);
    }
}

int is_stop_word(const char *word)
{
    // Duyệt qua toàn bộ danh sách các từ đã nạp vào stop_words
    for (int i = 0; i < stop_word_count; i++)
    {
        // So sánh chuỗi đầu vào 'word' với từ trong danh sách 'stop_words[i]'
        // Hàm strcmp trả về 0 nếu 2 chuỗi giống hệt nhau
        if (strcmp(stop_words[i], word) == 0)
        {
            return 1; // Trả về 1 (True) nếu là stop word -> Cần loại bỏ
        }
    }
    return 0; // Trả về 0 (False) nếu không tìm thấy -> Từ hợp lệ
}

// Hàm so sánh phụ trợ cho qsort
// Nhiệm vụ: So sánh 2 phần tử WordIndex dựa trên trường 'word'
int compare_word_index(const void *a, const void *b)
{
    // Ép kiểu con trỏ void* về kiểu struct WordIndex*
    const WordIndex *entryA = (const WordIndex *)a;
    const WordIndex *entryB = (const WordIndex *)b;

    // Dùng strcmp để so sánh thứ tự từ điển của 2 từ
    // strcmp trả về < 0 nếu A < B, > 0 nếu A > B, 0 nếu bằng nhau
    // Đây chính xác là thứ qsort cần.
    return strcmp(entryA->word, entryB->word);
}

// Hàm sắp xếp chính
void sort_index_table()
{
    // Gọi hàm qsort chuẩn của thư viện C
    // - index_table: Mảng cần sắp xếp
    // - index_count: Số lượng phần tử thực tế trong mảng
    // - sizeof(WordIndex): Kích thước của 1 phần tử (để qsort biết bước nhảy bộ nhớ)
    // - compare_word_index: Hàm so sánh logic do ta tự viết
    qsort(index_table, index_count, sizeof(WordIndex), compare_word_index);

    printf("[INFO] Table sorted. Total unique words: %d\n", index_count);
}
