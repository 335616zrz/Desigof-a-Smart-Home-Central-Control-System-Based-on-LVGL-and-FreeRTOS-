# Stage 1: Build with Maven
FROM maven:3.9.8-eclipse-temurin-17 AS build
WORKDIR /workspace
COPY deploy/settings.xml /root/.m2/settings.xml
COPY backend/pom.xml ./
RUN mvn -B dependency:go-offline
COPY backend/src ./src
RUN mvn -B clean package -DskipTests

# Stage 2: Runtime with JRE
FROM eclipse-temurin:17-jre-alpine
# Tools: ffmpeg/ffprobe for transcoding FLAC -> MP3 (WeChat mini program compatibility).
RUN apk add --no-cache ffmpeg

# Run as a non-root user. Use uid 1000 to match common host user ids for volume write access.
RUN adduser -D -u 1000 app
WORKDIR /app
COPY --from=build /workspace/target/iot-cloud-backend-0.1.0-SNAPSHOT.jar app.jar
USER app
EXPOSE 8080
ENTRYPOINT ["java", "-jar", "app.jar"]
