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

for entry in data:
    entry['file'] = re.sub(r'[\r\n]+', '', entry['file']).strip()
    entry['directory'] = re.sub(r'[\r\n]+', '', entry['directory']).strip()
    new_args = []
    for arg in entry.get('arguments', []):
        arg_clean = re.sub(r'[\r\n]+', '', arg).strip()
        if 'MonolithV.1.rsp' in arg_clean:
            new_args.extend(rsp_args)
        else:
            new_args.append(arg_clean)
    entry['arguments'] = new_args

s = json.dumps(data, indent=2)
paths = ['compile_commands.json', r'.vscode\compile_commands.json', r'game\Monolith_V\.vscode\compile_commands.json']
for path in paths:
    with open(path, 'w', encoding='utf-8') as out:
        out.write(s)
print('Successfully generated clean compile_commands.json for', len(data), 'entries')
