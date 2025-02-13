# AI and Multi-Agent Systems

### Useful commands

Bash
```bash
javac searchclient/*.java && java -jar ../server.jar -l ../levels/MAPF00.lvl -c "java -Xmx8g searchclient.SearchClient -heur zero -bfs" -g -s 150 -t 180 
```

Powershell
```powershell
javac searchclient/*.java ; java -jar ../server.jar -l ../levels/MAPF00.lvl -c "java -Xmx8g searchclient.SearchClient -heur zero -bfs" -g -s 150 -t 180 
```