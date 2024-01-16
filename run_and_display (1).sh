/bin/bash

# Compile the C program
gcc main.c weather.c -o weather_app -ljansson -lcurl

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Compilation successful"

    # Run the C program and redirect output to the terminal
    ./weather_app 2>&1

    # Check if the C program ran successfully
    if [ $? -eq 0 ]; then
        echo "C program executed successfully"

        # Display the content of the environmental report file
        echo -e "\nEnvironmental Report Data:"
        cat /home/maryam/Desktop/environmental_report.txt

        # Display the content of the alert JSON file
        echo -e "\nAlert JSON Data:"
        cat /home/maryam/Desktop/alert.json

        # Get the content of the alert JSON file
        alert_content=$(cat /home/maryam/Desktop/environmental_report.txt)

        # Create a temporary file for the alert content
        tmp_file=$(mktemp /tmp/alert_content.XXXXXX)
        echo "$alert_content" > "$tmp_file"

        # Send an email with the content of the alert JSON file using swaks
        swaks --to john36283@gmail.com \
              --from nomanmaryam385@gmail.com  \
              --server smtp.gmail.com:587 \
              --tls \
              --auth LOGIN \
              --auth-user nomanmaryam385@gmail.com \
              --auth-password "acdx dkey rofp iobw" \
              --header "Subject: Weather App Alert" \
              --body "$tmp_file"

        # Remove the temporary file
        rm -f "$tmp_file"

        echo "Email sent successfully"

    else
        echo "Error: C program execution failed"
    fi
else
    echo "Error: Compilation failed"
fi

