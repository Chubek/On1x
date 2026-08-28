#!/usr/bin/env python3
"""
gdesc2html.py - Compile GDesc grammar specifications to HTML
"""

import re
import sys
from pathlib import Path
from html import escape


class GDescParser:
    def __init__(self, content):
        self.content = content
        self.metadata = {}
        self.rules = []
        
    def parse(self):
        lines = self.content.split('\n')
        current_rule = None
        current_annotations = []
        
        for line in lines:
            line = line.rstrip()
            
            # Skip empty lines and comments
            if not line or line.strip().startswith('#'):
                continue
            
            # Parse metadata
            if line.strip().startswith('@') and '::=' not in line:
                match = re.match(r'@(\w+)\s+"([^"]+)"', line.strip())
                if match:
                    key, value = match.groups()
                    self.metadata[key] = value
                continue
            
            # Parse annotations for rules
            if line.strip().startswith('@') and current_rule:
                match = re.match(r'@(\w+)\s+"([^"]+)"', line.strip())
                if match:
                    key, value = match.groups()
                    current_annotations.append((key, value))
                continue
            
            # Parse production rules
            if '::=' in line:
                # Save previous rule if exists
                if current_rule:
                    current_rule['annotations'] = current_annotations
                    self.rules.append(current_rule)
                    current_annotations = []
                
                # Parse new rule
                match = re.match(r'\s*(\S+)\s*::=\s*(.+)', line)
                if match:
                    name, definition = match.groups()
                    current_rule = {
                        'name': name,
                        'definition': [definition.rstrip()],
                        'annotations': []
                    }
            elif line.strip() == ';':
                # End of rule
                if current_rule:
                    current_rule['annotations'] = current_annotations
                    self.rules.append(current_rule)
                    current_rule = None
                    current_annotations = []
            elif current_rule and line.strip():
                # Continuation of definition
                current_rule['definition'].append(line.strip())
        
        # Add last rule if exists
        if current_rule:
            current_rule['annotations'] = current_annotations
            self.rules.append(current_rule)
        
        return self


class HTMLGenerator:
    def __init__(self, parser):
        self.parser = parser
        
    def generate(self):
        html = self._header()
        html += self._metadata()
        html += self._grammar()
        html += self._footer()
        return html
    
    def _header(self):
        lang_name = self.parser.metadata.get('language', 'Grammar')
        return f'''<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{escape(lang_name)} - Grammar Specification</title>
    <style>
        * {{
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }}
        
        body {{
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            line-height: 1.6;
            color: #333;
            background: #f5f5f5;
            padding: 20px;
        }}
        
        .container {{
            max-width: 1200px;
            margin: 0 auto;
            background: white;
            padding: 40px;
            border-radius: 8px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }}
        
        header {{
            border-bottom: 3px solid #2c3e50;
            padding-bottom: 20px;
            margin-bottom: 30px;
        }}
        
        h1 {{
            color: #2c3e50;
            font-size: 2.5em;
            margin-bottom: 10px;
        }}
        
        .metadata {{
            background: #ecf0f1;
            padding: 20px;
            border-radius: 5px;
            margin-bottom: 30px;
        }}
        
        .metadata p {{
            margin: 8px 0;
        }}
        
        .metadata strong {{
            color: #34495e;
            min-width: 100px;
            display: inline-block;
        }}
        
        .metadata a {{
            color: #3498db;
            text-decoration: none;
        }}
        
        .metadata a:hover {{
            text-decoration: underline;
        }}
        
        .grammar {{
            margin-top: 30px;
        }}
        
        .rule {{
            margin-bottom: 30px;
            padding: 20px;
            background: #fafafa;
            border-left: 4px solid #3498db;
            border-radius: 4px;
        }}
        
        .rule-name {{
            color: #2980b9;
            font-weight: bold;
            font-size: 1.2em;
            font-family: 'Courier New', monospace;
            margin-bottom: 10px;
        }}
        
        .rule-definition {{
            font-family: 'Courier New', monospace;
            padding: 15px;
            background: white;
            border: 1px solid #ddd;
            border-radius: 3px;
            overflow-x: auto;
            white-space: pre-wrap;
            word-wrap: break-word;
        }}
        
        .rule-definition .operator {{
            color: #e74c3c;
            font-weight: bold;
        }}
        
        .rule-definition .literal {{
            color: #27ae60;
        }}
        
        .rule-definition .terminal {{
            color: #8e44ad;
        }}
        
        .annotations {{
            margin-top: 15px;
        }}
        
        .annotation {{
            padding: 8px 12px;
            margin: 5px 0;
            border-radius: 3px;
            font-size: 0.9em;
        }}
        
        .annotation.note {{
            background: #d5e8f7;
            border-left: 3px solid #3498db;
        }}
        
        .annotation.warning {{
            background: #fef5e7;
            border-left: 3px solid #f39c12;
        }}
        
        .annotation.credit {{
            background: #e8f5e9;
            border-left: 3px solid #27ae60;
        }}
        
        .annotation.repl {{
            background: #f3e5f5;
            border-left: 3px solid #9b59b6;
        }}
        
        .annotation-label {{
            font-weight: bold;
            text-transform: uppercase;
            font-size: 0.8em;
            margin-right: 8px;
        }}
        
        footer {{
            margin-top: 50px;
            padding-top: 20px;
            border-top: 1px solid #ddd;
            text-align: center;
            color: #7f8c8d;
            font-size: 0.9em;
        }}
        
        @media print {{
            body {{
                background: white;
            }}
            .container {{
                box-shadow: none;
            }}
        }}
    </style>
</head>
<body>
    <div class="container">
'''
    
    def _metadata(self):
        html = '<header>\n'
        html += f'<h1>{escape(self.parser.metadata.get("language", "Grammar Specification"))}</h1>\n'
        html += '</header>\n'
        
        html += '<div class="metadata">\n'
        
        if 'author' in self.parser.metadata:
            html += f'<p><strong>Author:</strong> {escape(self.parser.metadata["author"])}</p>\n'
        
        if 'homepage' in self.parser.metadata:
            homepage = self.parser.metadata['homepage']
            html += f'<p><strong>Homepage:</strong> <a href="{escape(homepage)}">{escape(homepage)}</a></p>\n'
        
        if 'summary' in self.parser.metadata:
            html += f'<p><strong>Summary:</strong> {escape(self.parser.metadata["summary"])}</p>\n'
        
        html += '</div>\n'
        
        return html
    
    def _format_definition(self, definition):
        """Format a grammar definition with syntax highlighting"""
        formatted = []
        
        for line in definition:
            # Highlight operators
            line = re.sub(r'(\||::=|\(|\)|\[|\]|\+|\*|\?)', r'<span class="operator">\1</span>', line)
            
            # Highlight quoted literals
            line = re.sub(r'"([^"]*)"', r'<span class="literal">"\1"</span>', line)
            line = re.sub(r"'([^']*)'", r'<span class="literal">\'\1\'</span>', line)
            
            formatted.append(line)
        
        return '\n'.join(formatted)
    
    def _grammar(self):
        html = '<div class="grammar">\n'
        html += '<h2>Grammar Rules</h2>\n'
        
        for rule in self.parser.rules:
            html += '<div class="rule">\n'
            html += f'<div class="rule-name">{escape(rule["name"])}</div>\n'
            
            definition = ' '.join(rule['definition'])
            formatted_def = self._format_definition([definition])
            html += f'<div class="rule-definition">{formatted_def}</div>\n'
            
            if rule['annotations']:
                html += '<div class="annotations">\n'
                for key, value in rule['annotations']:
                    html += f'<div class="annotation {key}">\n'
                    html += f'<span class="annotation-label">{key}:</span>\n'
                    html += f'{escape(value)}\n'
                    html += '</div>\n'
                html += '</div>\n'
            
            html += '</div>\n'
        
        html += '</div>\n'
        return html
    
    def _footer(self):
        return '''
        <footer>
            <p>Generated by gdesc2html.py</p>
        </footer>
    </div>
</body>
</html>
'''


def main():
    if len(sys.argv) < 2:
        print("Usage: gdesc2html.py <input.gdesc> [output.html]")
        print("  If output.html is not specified, writes to <input>.html")
        sys.exit(1)
    
    input_file = Path(sys.argv[1])
    
    if not input_file.exists():
        print(f"Error: File '{input_file}' not found")
        sys.exit(1)
    
    # Determine output file path
    if len(sys.argv) >= 3:
        output_file = Path(sys.argv[2])
    else:
        output_file = input_file.with_suffix('.html')
    
    try:
        # Read and parse GDesc source
        with open(input_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        parser = GDescParser(content)
        parser.parse()
        
        # Generate HTML output
        generator = HTMLGenerator(parser)
        html_output = generator.generate()
        
        # Write output file
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(html_output)
        
        print(f"Successfully compiled '{input_file}' to '{output_file}'")
        
    except Exception as e:
        print(f"Error processing file: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
