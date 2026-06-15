#!/bin/bash
# Copy this to deploy.sh (gitignored) and fill in your server details.
# Run after building: bash build.sh && bash deploy.sh

DEPLOY_USER=youruser
DEPLOY_HOST=yourserver.example.com
DEPLOY_PATH=/var/www/html/e2gen

set -e
rsync -avz --exclude=bak/ html/ ${DEPLOY_USER}@${DEPLOY_HOST}:${DEPLOY_PATH}/
ssh ${DEPLOY_USER}@${DEPLOY_HOST} "chmod +x ${DEPLOY_PATH}/*.cgi"

# Note: *-list.txt files (used by req-b10x10s1.cgi) are too large for git.
# Deploy them once manually:
#   rsync -avz html/*-list.txt ${DEPLOY_USER}@${DEPLOY_HOST}:${DEPLOY_PATH}/
