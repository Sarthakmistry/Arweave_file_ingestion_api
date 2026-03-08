const fs = require('fs');
const Arweave = require('arweave');

// Connect to the local ArLocal testnet running on port 1984
const arweave = Arweave.init({
    host: 'localhost',
    port: 1984,
    protocol: 'http'
});

async function uploadFile() {
    try {
        const filePath = process.argv[2];
        if (!filePath) {
            console.error("Usage: node arweave_upload.js <file_to_upload>");
            process.exit(1);
        }

        const data = fs.readFileSync(filePath);

        // 1. Generate a temporary test wallet instantly
        const key = await arweave.wallets.generate();
        const walletAddress = await arweave.wallets.jwkToAddress(key);

        // 2. Airdrop free test AR tokens to this wallet
        await arweave.api.get(`/mint/${walletAddress}/10000000000000`); 

        // 3. Create and sign the transaction
        const transaction = await arweave.createTransaction({ data: data }, key);
        transaction.addTag('Content-Type', 'text/plain'); 
        await arweave.transactions.sign(transaction, key);

        // 4. Upload the data
        let uploader = await arweave.transactions.getUploader(transaction);
        while (!uploader.isComplete) {
            await uploader.uploadChunk();
        }

        // 5. Force the local testnet to mine a block (instant confirmation)
        await arweave.api.get('/mine');

        // Output ONLY the transaction ID so C++ can capture it cleanly
        process.stdout.write(transaction.id);

    } catch (error) {
        console.error("Upload failed:", error.message);
        process.exit(1);
    }
}

uploadFile();