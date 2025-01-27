
import re
import os

# Define the path to the brilcalc.log file
file_path = "/mnt/c/Users/Omistaja/Documents/CompinerHEP2025/Exercises/Ex2/returned_answers/matildamarjamaki/brilcalc.log"

# Check if the file exists
if os.path.exists(file_path):
    try:
        # Open and read the file
        with open(file_path, "r") as file:
            content = file.read()

        # Use regex to find all the recorded luminosities in /pb for different run:fill
        luminosities_pb = re.findall(r"\|.*\|.*\|.*\|.*\|.*\|\s+([\d\.]+)", content)

        if luminosities_pb:
            # Convert to floats and calculate the sum of the luminosities
            luminosities_pb = [float(lum) for lum in luminosities_pb]
            luminosities_pb.pop()
            total_luminosity_pb = sum(luminosities_pb)

            # Convert to /fb and format to one decimal precision
            total_luminosity_fb = total_luminosity_pb / 1000
            print(f"Total recorded luminosity: {total_luminosity_fb:.1f} fb^-1")

            # Check if the total luminosity matches the summary value
            # Use regex to extract the summary luminosity value
            summary_match = re.search(r"totrecorded\(/pb\)\s+\|\n.*\n\|.*\|.*\|.*\|.*\|.*\|\s+([\d\.]+)", content)
            if summary_match:
                summary_luminosity_pb = float(summary_match.group(1))
                summary_luminosity_fb = summary_luminosity_pb / 1000
                print(f"Summary luminosity: {summary_luminosity_fb:.1f} fb^-1")

                if abs(total_luminosity_fb - summary_luminosity_fb) < 0.1:  # Allowing for small rounding differences
                    print("The total luminosity matches the summary luminosity.")
                else:
                    print("The total luminosity does not match the summary luminosity.")
            else:
                print("Could not find the summary 'totrecorded(/pb)' value in the file.")
        else:
            print("Could not find any 'totrecorded(/pb)' values in the file.")

    except Exception as e:
        print(f"An error occurred: {e}")
else:
    print(f"File not found: {file_path}")