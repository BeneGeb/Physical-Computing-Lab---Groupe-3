FROM node:20-slim
WORKDIR /app
COPY package.json ./
RUN apt-get update && apt-get install -y zbar-tools
RUN npm install
COPY . .
EXPOSE 5000
CMD ["npm", "start"]
