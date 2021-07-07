set ports=8061 8062 8063 8065 8064 8066 8067 8068 8069 8074 8072 8071 8073

for %%a in (%ports%) do (
  start cmd.exe /c .\build\Debug\BullyAlgoNode.exe %%a
)