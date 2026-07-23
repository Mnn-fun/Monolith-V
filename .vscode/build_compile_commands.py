import json
import re
import os

rsp_path = r'game\Monolith_V\.vscode\compileCommands_Monolith_V\MonolithV.1.rsp'
lines = [l.strip() for l in open(rsp_path, encoding='utf-8', errors='ignore') if l.strip()]
rsp_args = []
for l in lines:
    m = re.match(r'^(/FI|/I|/D|/std:[^\s]+)\s*(.*)$', l)
    if m:
        flag, val = m.group(1), m.group(2).strip().strip('"')
        rsp_args.append(flag)
        if val:
            rsp_args.append(val)
    else:
        rsp_args.append(l.strip())

json_path = r'game\Monolith_V\.vscode\compileCommands_Monolith_V.json'
with open(json_path, encoding='utf-8', errors='ignore') as f:
    data = json.load(f)

existing_files = set()
base_args = []
base_directory = r'C:\Program Files\Epic Games\UE_5.7\Engine\Source'

for entry in data:
    entry['file'] = re.sub(r'[\r\n]+', '', entry['file']).strip()
    entry['directory'] = re.sub(r'[\r\n]+', '', entry['directory']).strip()
    existing_files.add(os.path.abspath(entry['file']).lower())
    new_args = []
    for arg in entry.get('arguments', []):
        arg_clean = re.sub(r'[\r\n]+', '', arg).strip()
        if 'MonolithV.1.rsp' in arg_clean:
            new_args.extend(rsp_args)
        else:
            new_args.append(arg_clean)
    if new_args and os.path.abspath(entry['file']).lower() != os.path.abspath(new_args[-1]).lower():
        new_args.append(entry['file'])
    entry['arguments'] = new_args
    if not base_args and new_args:
        base_args = [a for a in new_args if not a.endswith('.cpp') and not a.endswith('.h')]
        base_directory = entry['directory']

source_dir = os.path.abspath(r'game\Monolith_V\Source\MonolithV')
for root, _, files in os.walk(source_dir):
    for f in files:
        if f.endswith('.cpp') or f.endswith('.h'):
            fpath = os.path.join(root, f)
            if os.path.abspath(fpath).lower() not in existing_files:
                file_args = list(base_args) + [fpath]
                data.append({
                    'file': fpath,
                    'arguments': file_args,
                    'directory': base_directory
                })
                existing_files.add(os.path.abspath(fpath).lower())

s = json.dumps(data, indent=2)
paths = ['compile_commands.json', r'.vscode\compile_commands.json', r'game\Monolith_V\.vscode\compile_commands.json']
for path in paths:
    with open(path, 'w', encoding='utf-8') as out:
        out.write(s)
print('Successfully generated clean compile_commands.json for', len(data), 'entries')


