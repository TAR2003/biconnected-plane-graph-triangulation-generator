from PyPDF2 import PdfReader

# Input and output file paths
pdf_path = "./ParvezRahmanNakano2011.15.3.pdf"
md_path = "./ParvezRahmanNakano.md"

# Open the PDF
reader = PdfReader(pdf_path)

# Extract text
all_text = ""
for page in reader.pages:
    all_text += page.extract_text() + "\n\n"

# Write to Markdown file
with open(md_path, "w", encoding="utf-8") as f:
    f.write(all_text)

print(f"✅ Text extracted and saved to {md_path}")
