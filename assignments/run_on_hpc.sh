#!/bin/bash

# Version 0 - assumes you have an ssh key for login1.hpc.dtu.dk under ~/.ssh/id_rsa_hpc
#             and that you have a project directory in your home directory
#             and that you have java 17 installed and using it to build the searchclient
#             and that all the files exist
#             and that you created the proj structer on your hpc node

SSH_KEY_PATH="$HOME/.ssh/id_rsa_hpc"

# Select the level file
read -p "Enter the level file name (e.g., MAPF01.lvl): " LEVEL_FILE
# LEVEL_FILE="MAPF02.lvl"

# Define variables
JAR_FILE="server.jar"
LOCAL_PROJ_PATH="$HOME/projects/ai-multi-agent-systems"
LOCAL_LEVELS_PATH="$LOCAL_PROJ_PATH/levels"
LOCAL_SEARCHCLIENT_PATH="$LOCAL_PROJ_PATH/searchclient_java/searchclient.jar"

REMOTE_PROJ_PATH="projects/ai-multi-agent-systems"
REMOTE_JAR_PATH="$REMOTE_PROJ_PATH/$JAR_FILE"
REMOTE_LEVELS_PATH="$REMOTE_PROJ_PATH/levels"
REMOTE_SEARCHCLIENT_PATH="$REMOTE_PROJ_PATH/searchclient.jar"
JAVA_COMMAND="cd $REMOTE_SEARCHCLIENT_PATH && java -jar $REMOTE_JAR_PATH -l $REMOTE_LEVELS_PATH/$LEVEL_FILE -c \"java -Xmx8g -jar searchclient.jar -bfs\" -t 180"

echo "JAVA_COMMAND: $JAVA_COMMAND"

# Prompt for username
read -p "Enter your student username (e.g., sXXXXXX): " USERNAME
HOST="$USERNAME@login1.hpc.dtu.dk"

# Copy the JAR and level files to the remote file system
echo "Copying files to the remote file system..."
echo "  Copying $LOCAL_PROJ_PATH/$JAR_FILE to $HOST:$REMOTE_JAR_PATH"
scp -i "$SSH_KEY_PATH" "$LOCAL_PROJ_PATH/$JAR_FILE" "$HOST:$REMOTE_JAR_PATH"
echo "  Copying $LOCAL_LEVELS_PATH/$LEVEL_FILE to $HOST:$REMOTE_LEVELS_PATH/$LEVEL_FILE"
scp -i "$SSH_KEY_PATH" "$LOCAL_LEVELS_PATH/$LEVEL_FILE" "$HOST:$REMOTE_LEVELS_PATH/$LEVEL_FILE"
echo "  Copying $LOCAL_SEARCHCLIENT_PATH to $HOST:$REMOTE_SEARCHCLIENT_PATH"
scp -i "$SSH_KEY_PATH" "$LOCAL_SEARCHCLIENT_PATH" "$HOST:$REMOTE_SEARCHCLIENT_PATH"

# SSH into the login node and execute the commands
echo "Connecting to the login node and running the program..."
# TODO: currently do that part manually, from the hpc node
ssh -i "$SSH_KEY_PATH" "$HOST" << EOF
    # linuxsh

    # # Navigate to the directory where you copied the files (needed for relative paths)
    # cd $REMOTE_PROJ_PATH

    # # Run the Java program using linuxsh and capture the output
    # OUTPUT="\$JAVA_COMMAND"

    # # Print the output
    # echo "\$OUTPUT"

    echo "Java program execution complete."
EOF

echo "Script finished."