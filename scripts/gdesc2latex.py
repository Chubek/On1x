#!/usr/bin/env python3
"""
gdesc2latex.py - Compile GDesc grammar specifications to LaTeX
"""

import re
import sys
from pathlib import Path


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


class LaTeXGenerator:
    def __init__(self, parser):
        self.parser = parser
        
    def generate(self):
        latex = self._preamble()
        latex += self._begin_document()
        latex += self._title_page()
        latex += self._metadata()
        latex += self._grammar()
        latex += self._end_document()
        return latex
    
    def _escape(self, text):
        """Escape special LaTeX characters"""
        replacements = {
            '\\': r'\textbackslash{}',
            '{': r'\{',
            '}': r'\}',
            '$': r'\$',
            '&': r'\&',
            '%': r'\%',
            '#': r'\#',
            '_': r'\_',
            '~': r'\textasciitilde{}',
            '^': r'\textasciicircum{}'
        }
        for char, escaped in replacements.items():
            text = text.replace(char, escaped)
        return text
    
    def _preamble(self):
        return r'''\documentclass[11pt,a4paper]{article}
\usepackage[utf8]{inputenc}
\usepackage[margin=1in]{geometry}
\usepackage{xcolor}
\usepackage{listings}
\usepackage{tcolorbox}
\usepackage{hyperref}
\usepackage{fancyhdr}
\usepackage{titlesec}

% Define colors
\definecolor{rulename}{RGB}{41,128,185}
\definecolor{operator}{RGB}{231,76,60}
\definecolor{literal}{RGB}{39,174,96}
\definecolor{terminal}{RGB}{142,68,173}
\definecolor{notebg}{RGB}{213,232,247}
\definecolor{warningbg}{RGB}{254,245,231}
\definecolor{creditbg}{RGB}{232,245,233}
\definecolor{replbg}{RGB}{243,229,245}

% Configure hyperlinks
\hypersetup{
    colorlinks=true,
    linkcolor=blue,
    urlcolor=blue,
    citecolor=blue
}

% Configure listings for grammar rules
\lstdefinestyle{grammar}{
    basicstyle=\ttfamily\small,
    breaklines=true,
    breakatwhitespace=false,
    frame=single,
    backgroundcolor=\color{white},
    numbers=none,
    xleftmargin=0.5cm,
    xrightmargin=0.5cm
}

% Custom title formatting
\titleformat{\section}
  {\normalfont\Large\bfseries\color{rulename}}
  {\thesection}{1em}{}

\titleformat{\subsection}
  {\normalfont\large\bfseries\color{rulename}}
  {\thesubsection}{1em}{}

% Page style
\pagestyle{fancy}
\fancyhf{}
\rhead{\thepage}
\lhead{Grammar Specification}
\renewcommand{\headrulewidth}{0.4pt}

'''
    
    def _begin_document(self):
        return r'\begin{document}' + '\n\n'
    
    def _title_page(self):
        lang_name = self._escape(self.parser.metadata.get('language', 'Grammar Specification'))
        latex = r'\begin{titlepage}' + '\n'
        latex += r'\centering' + '\n'
        latex += r'\vspace*{2cm}' + '\n'
        latex += r'{\Huge\bfseries ' + lang_name + r'}' + '\n\n'
        latex += r'\vspace{1cm}' + '\n'
        latex += r'{\Large Grammar Specification}' + '\n\n'
        latex += r'\vfill' + '\n'
        
        if 'author' in self.parser.metadata:
            author = self._escape(self.parser.metadata['author'])
            latex += r'{\large ' + author + r'}' + '\n\n'
        
        latex += r'\vspace{1cm}' + '\n'
        latex += r'{\large Generated by gdesc2latex.py}' + '\n'
        latex += r'\end{titlepage}' + '\n\n'
        
        return latex
    
    def _metadata(self):
        if not self.parser.metadata:
            return ''
        
        latex = r'\section*{Metadata}' + '\n\n'
        latex += r'\begin{description}' + '\n'
        
        if 'language' in self.parser.metadata:
            lang = self._escape(self.parser.metadata['language'])
            latex += r'\item[Language:] ' + lang + '\n'
        
        if 'author' in self.parser.metadata:
            author = self._escape(self.parser.metadata['author'])
            latex += r'\item[Author:] ' + author + '\n'
        
        if 'homepage' in self.parser.metadata:
            homepage = self.parser.metadata['homepage']
            latex += r'\item[Homepage:] \url{' + homepage + r'}' + '\n'
        
        if 'summary' in self.parser.metadata:
            summary = self._escape(self.parser.metadata['summary'])
            latex += r'\item[Summary:] ' + summary + '\n'
        
        latex += r'\end{description}' + '\n\n'
        latex += r'\clearpage' + '\n\n'
        
        return latex
    
    def _format_definition(self, definition):
        """Format a grammar definition for LaTeX"""
        # Join all parts
        full_def = ' '.join(definition)
        
        # Escape the definition
        escaped = self._escape(full_def)
        
        return escaped
    
    def _grammar(self):
        latex = r'\section{Grammar Rules}' + '\n\n'
        
        for rule in self.parser.rules:
            # Rule name as subsection
            rule_name = self._escape(rule['name'])
            latex += r'\subsection*{\texttt{\color{rulename}' + rule_name + r'}}' + '\n\n'
            
            # Rule definition in a box
            definition = self._format_definition(rule['definition'])
            latex += r'\begin{tcolorbox}[colback=white,colframe=gray!30,boxrule=0.5pt]' + '\n'
            latex += r'\begin{lstlisting}[style=grammar]' + '\n'
            latex += definition + '\n'
            latex += r'\end{lstlisting}' + '\n'
            latex += r'\end{tcolorbox}' + '\n\n'
            
            # Annotations
            if rule['annotations']:
                for key, value in rule['annotations']:
                    color_map = {
                        'note': 'notebg',
                        'warning': 'warningbg',
                        'credit': 'creditbg',
                        'repl': 'replbg'
                    }
                    bgcolor = color_map.get(key, 'notebg')
                    
                    escaped_value = self._escape(value)
                    latex += r'\begin{tcolorbox}[colback=' + bgcolor + r',colframe=' + bgcolor + r'!80!black,' + '\n'
                    latex += r'boxrule=0.5pt,left=3pt,title={\textbf{\small ' + key.upper() + r'}}]' + '\n'
                    latex += r'\small ' + escaped_value + '\n'
                    latex += r'\end{tcolorbox}' + '\n\n'
            
            latex += r'\vspace{0.5cm}' + '\n\n'
        
        return latex
    
    def _end_document(self):
        return r'\end{document}' + '\n'


def main():
    if len(sys.argv) < 2:
        print("Usage: gdesc2latex.py <input.gdesc> [output.tex]")
        print("  If output.tex is not specified, writes to <input>.tex")
        sys.exit(1)
    
    input_file = Path(sys.argv[1])
    
    if not input_file.exists():
        print(f"Error: File '{input_file}' not found")
        sys.exit(1)
    
    # Determine output file path
    if len(sys.argv) >= 3:
        output_file = Path(sys.argv[2])
    else:
        output_file = input_file.with_suffix('.tex')
    
    try:
        # Read and parse GDesc source
        with open(input_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        parser = GDescParser(content)
        parser.parse()
        
        # Generate LaTeX output
        generator = LaTeXGenerator(parser)
        latex_output = generator.generate()
        
        # Write output file
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(latex_output)
        
        print(f"Successfully compiled '{input_file}' to '{output_file}'")
        print(f"To generate PDF, run: pdflatex {output_file}")
        
    except Exception as e:
        print(f"Error processing file: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
