import { RpcError } from 'eosjs';
import { createCol, createAsset } from './actions.js';

(async () => {
  try {
    const collection = {
      author: 'nftbox.defi',
      royalty: 100, // 1%
      name: 'Defibox 2nd Anniversary NFT',
      description: '100+ NFTs for the anniversary event of Defibox each year. By holding NFTs you can participate NFT mining every year.',
      image: 'https://ipfs.io/ipfs/bafybeibriq6c3it3f3sxw2axjfxqrxct47vlyikcilflpmgy4bxxxuuqvq/ijlo5oor0kyfo6cn.png',
      banner: 'https://ipfs.io/ipfs/bafybeighzhgjmndtxondfehokdr3dj3226lo3fj3h6euyhplgm4w2gcfzq/totcpyf6p3fzaplu.png',
      links: {
        url: 'https://defibox.io/',
        twitter: 'https://twitter.com/DefiboxOfficial',
      },
    };
    const result = await createCol(collection);
    console.log(result);

    await  new Promise(resolve => setTimeout(resolve, 2000));

    for (let i = 1; i <= 50; i++) {
      const asset = {
        collection_id: 1,
        supply: 1,
        max_supply: 1,
        name: 'Defibox 2nd Anniversary NFT #' + i,
        description: 'Defibox 2nd Anniversary NFT',
        image: 'https://ipfs.io/ipfs/bafybeigzwjipmn3tiotladoi3h6l2grf6rugbvj4sq2sd4647sjbmqvqau/aq5yzotkg5ngribe.jpg',
        // animation_url: '',
        // external_url: '',
        attributes: [
          { trait_type: 'number', value: '#' + i },
        ],
      };
      const result = await createAsset(asset);
      console.log(result);
    }
  } catch (e) {
    if (e instanceof RpcError) {
      const err = e.json.error;
      console.log('EOS error:', err.name + ',', err.what);
      console.log('Error detail:', err.details[0].message);
    } else {
      console.log(e);
    }
  }
})();