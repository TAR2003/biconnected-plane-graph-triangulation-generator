import java.math.BigInteger;
import java.security.SecureRandom;
import java.util.Random;

/**
 * ElGamal Encryption System Implementation
 * 
 * This class provides a basic implementation of the ElGamal public-key cryptosystem
 * including key generation, encryption, and decryption functionality.
 */
public class ElGamalEncryption {
    
    private BigInteger p;  // Large prime number
    private BigInteger g;  // Generator
    private BigInteger x;  // Private key
    private BigInteger y;  // Public key
    
    private SecureRandom random;
    
    /**
     * Constructor that generates ElGamal keys
     * @param bitLength Bit length for the prime number (recommended: 512-2048 bits)
     */
    public ElGamalEncryption(int bitLength) {
        this.random = new SecureRandom();
        generateKeys(bitLength);
    }
    
    /**
     * Generate ElGamal public and private keys
     * @param bitLength Bit length for the prime number
     */
    private void generateKeys(int bitLength) {
        // Step 1: Generate a large prime number p
        p = BigInteger.probablePrime(bitLength, random);
        
        // Step 2: Find a generator g for the multiplicative group modulo p
        g = findGenerator(p);
        
        // Step 3: Choose a private key x (1 < x < p-1)
        x = new BigInteger(bitLength - 1, random);
        while (x.compareTo(BigInteger.ONE) <= 0 || x.compareTo(p.subtract(BigInteger.ONE)) >= 0) {
            x = new BigInteger(bitLength - 1, random);
        }
        
        // Step 4: Compute public key y = g^x mod p
        y = g.modPow(x, p);
        
        System.out.println("Keys generated successfully:");
        System.out.println("Prime p: " + p.toString(16));
        System.out.println("Generator g: " + g.toString(16));
        System.out.println("Private key x: " + x.toString(16));
        System.out.println("Public key y: " + y.toString(16));
    }
    
    /**
     * Find a generator for the multiplicative group modulo p
     * @param p Prime number
     * @return A generator g
     */
    private BigInteger findGenerator(BigInteger p) {
        BigInteger g;
        BigInteger pMinusOne = p.subtract(BigInteger.ONE);
        
        // Try random numbers until we find a generator
        do {
            g = new BigInteger(p.bitLength() - 1, random);
            // Ensure g is between 2 and p-2
            if (g.compareTo(BigInteger.ONE) <= 0 || g.compareTo(pMinusOne) >= 0) {
                continue;
            }
        } while (!isGenerator(g, p, pMinusOne));
        
        return g;
    }
    
    /**
     * Check if g is a generator modulo p
     * @param g Candidate generator
     * @param p Prime number
     * @param pMinusOne p-1
     * @return true if g is a generator, false otherwise
     */
    private boolean isGenerator(BigInteger g, BigInteger p, BigInteger pMinusOne) {
        // For a prime p, g is a generator if g^((p-1)/q) != 1 mod p for all prime factors q of p-1
        // For simplicity, we'll use a probabilistic test
        BigInteger result = g.modPow(pMinusOne.divide(BigInteger.valueOf(2)), p);
        return !result.equals(BigInteger.ONE);
    }
    
    /**
     * Encrypt a message using ElGamal encryption
     * @param message The message to encrypt (as a BigInteger)
     * @return A pair (c1, c2) representing the ciphertext
     */
    public BigInteger[] encrypt(BigInteger message) {
        if (message.compareTo(p) >= 0) {
            throw new IllegalArgumentException("Message must be smaller than p");
        }
        
        // Choose a random k (1 < k < p-1)
        BigInteger k;
        do {
            k = new BigInteger(p.bitLength() - 1, random);
        } while (k.compareTo(BigInteger.ONE) <= 0 || k.compareTo(p.subtract(BigInteger.ONE)) >= 0);
        
        // Compute c1 = g^k mod p
        BigInteger c1 = g.modPow(k, p);
        
        // Compute c2 = message * y^k mod p
        BigInteger yk = y.modPow(k, p);
        BigInteger c2 = message.multiply(yk).mod(p);
        
        return new BigInteger[]{c1, c2};
    }
    
    /**
     * Encrypt a string message
     * @param message The string message to encrypt
     * @return A pair (c1, c2) representing the ciphertext
     */
    public BigInteger[] encrypt(String message) {
        byte[] bytes = message.getBytes();
        BigInteger messageInt = new BigInteger(1, bytes);
        return encrypt(messageInt);
    }
    
    /**
     * Decrypt a ciphertext using ElGamal decryption
     * @param ciphertext The ciphertext pair (c1, c2)
     * @return The decrypted message as a BigInteger
     */
    public BigInteger decrypt(BigInteger[] ciphertext) {
        if (ciphertext.length != 2) {
            throw new IllegalArgumentException("Ciphertext must be a pair of BigIntegers");
        }
        
        BigInteger c1 = ciphertext[0];
        BigInteger c2 = ciphertext[1];
        
        // Compute s = c1^x mod p
        BigInteger s = c1.modPow(x, p);
        
        // Compute s^-1 mod p
        BigInteger sInverse = s.modInverse(p);
        
        // Compute message = c2 * s^-1 mod p
        BigInteger message = c2.multiply(sInverse).mod(p);
        
        return message;
    }
    
    /**
     * Decrypt a ciphertext and convert to string
     * @param ciphertext The ciphertext pair (c1, c2)
     * @return The decrypted string message
     */
    public String decryptToString(BigInteger[] ciphertext) {
        BigInteger messageInt = decrypt(ciphertext);
        byte[] messageBytes = messageInt.toByteArray();
        
        // Remove leading zero byte if present
        if (messageBytes[0] == 0 && messageBytes.length > 1) {
            byte[] trimmedBytes = new byte[messageBytes.length - 1];
            System.arraycopy(messageBytes, 1, trimmedBytes, 0, trimmedBytes.length);
            return new String(trimmedBytes);
        }
        
        return new String(messageBytes);
    }
    
    /**
     * Get the public key components (p, g, y)
     * @return Array containing [p, g, y]
     */
    public BigInteger[] getPublicKey() {
        return new BigInteger[]{p, g, y};
    }
    
    /**
     * Set public key components (for testing or external key usage)
     * @param p Prime number
     * @param g Generator
     * @param y Public key
     */
    public void setPublicKey(BigInteger p, BigInteger g, BigInteger y) {
        this.p = p;
        this.g = g;
        this.y = y;
    }
    
    /**
     * Set private key (for testing or external key usage)
     * @param x Private key
     */
    public void setPrivateKey(BigInteger x) {
        this.x = x;
    }
    
    /**
     * Main method to demonstrate ElGamal encryption
     */
    public static void main(String[] args) {
        System.out.println("=== ElGamal Encryption System Demo ===\n");
        
        // Create ElGamal instance with 512-bit security (use higher for production)
        ElGamalEncryption elgamal = new ElGamalEncryption(512);
        
        // Original message
        String originalMessage = "Hello, ElGamal Encryption!";
        System.out.println("Original message: " + originalMessage);
        
        // Encrypt the message
        BigInteger[] ciphertext = elgamal.encrypt(originalMessage);
        System.out.println("\nCiphertext:");
        System.out.println("c1: " + ciphertext[0].toString(16));
        System.out.println("c2: " + ciphertext[1].toString(16));
        
        // Decrypt the message
        String decryptedMessage = elgamal.decryptToString(ciphertext);
        System.out.println("\nDecrypted message: " + decryptedMessage);
        
        // Verify the decryption
        if (originalMessage.equals(decryptedMessage)) {
            System.out.println("\n✓ Encryption and decryption successful!");
        } else {
            System.out.println("\n✗ Encryption/decryption failed!");
        }
        
        // Demonstrate with numeric message
        System.out.println("\n=== Numeric Message Example ===");
        BigInteger numericMessage = new BigInteger("123456789");
        System.out.println("Numeric message: " + numericMessage);
        
        BigInteger[] numericCiphertext = elgamal.encrypt(numericMessage);
        BigInteger decryptedNumeric = elgamal.decrypt(numericCiphertext);
        System.out.println("Decrypted numeric: " + decryptedNumeric);
        
        if (numericMessage.equals(decryptedNumeric)) {
            System.out.println("✓ Numeric encryption/decryption successful!");
        }
    }
}