#!/bin/bash

OUTPUT_FILE="all_code_dump.txt"
> "$OUTPUT_FILE" # Xóa trắng file nếu đã tồn tại

# Tìm tất cả các file code, bỏ qua các thư mục log và build của Yocto
find . -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.qml" \) \
    -not -path "*/oe-logs/*" \
    -not -path "*/oe-workdir/*" \
    -not -path "*/build/*" | while read -r file; do
    
    echo "==================================================" >> "$OUTPUT_FILE"
    echo "FILE: $file" >> "$OUTPUT_FILE"
    echo "==================================================" >> "$OUTPUT_FILE"
    cat "$file" >> "$OUTPUT_FILE"
    echo -e "\n\n" >> "$OUTPUT_FILE"
done

echo "Đã trích xuất toàn bộ mã nguồn vào file: $OUTPUT_FILE"
