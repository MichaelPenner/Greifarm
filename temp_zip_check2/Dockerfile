ARG BUILD_FROM=ghcr.io/hassio-addons/base:14.0.0
FROM $BUILD_FROM

# Install python and sqlite
RUN apk add --no-cache python3 py3-pip sqlite

# Copy the project files into the container
WORKDIR /app
COPY backend/ /app/backend/
COPY frontend/ /app/frontend/

# Install python dependencies
RUN pip3 install --break-system-packages -r /app/backend/requirements.txt

# Copy and setup the run script
COPY run.sh /
RUN chmod a+x /run.sh

CMD [ "/run.sh" ]
