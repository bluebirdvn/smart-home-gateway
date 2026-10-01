#!/bin/bash

echo "find image.."

IMAGE_FILE=$(ls -t ./build/tmp*/deploy/images/raspberrypi0-2w-64/*.wic 2>/dev/null | head -n 1)

if [ -z "$IMAGE_FILE" ]; then
    echo "don't find out image"
    exit 1
fi

echo "found: $IMAGE_FILE"

echo "please plug sd card"

OLD_DRIVES=$(lsblk -d -n -o NAME)

while true; do
    NEW_DRIVES=$(lsblk -d -n -o NAME)

    for drive in ${NEW_DRIVES}; do
        if ! echo "${OLD_DRIVES}" | grep -q "^${drive}$"; then
            IS_REMOVABLE=$(cat /sys/block/"${drive}"/removable 2>/dev/null)

            if [ "$IS_REMOVABLE" == "1" ]; then
                TARGET_DEV="/dev/${drive}"
                DRIVE_SIZE=$(lsblk -d -n -o SIZE "${TARGET_DEV}")
                
                echo -e "\nfound sd: **${TARGET_DEV}** (cap: ${DRIVE_SIZE})"
                
                read -r -p "do you want erase all ${TARGET_DEV} and flash image? (y/N): " confirm
                if [[ $confirm == [yY] || $confirm == [yY][eE][sS] ]]; then
                    echo -e "flash image\n"
                    
                    sudo umount "${TARGET_DEV}"* 2>/dev/null || true
                    
                    sudo dd if="${IMAGE_FILE}" of="${TARGET_DEV}" bs=4M status=progress oflag=sync
                    
                    echo "flash success"
                    
                    exit 0
                else
                    echo "cancel"
                    exit 0
                fi
            fi
        fi
    done
    sleep 2
done