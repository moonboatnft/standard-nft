import { Api, JsonRpc } from 'eosjs';
import { JsSignatureProvider } from 'eosjs/dist/eosjs-jssig.js';
import fetch from 'node-fetch'; 
import { TextEncoder, TextDecoder } from 'util';

const defaultPrivateKey = 'Your private key';
const signatureProvider = new JsSignatureProvider([defaultPrivateKey]);

const rpc = new JsonRpc('https://eospush.tokenpocket.pro', { fetch });
const api = new Api({ rpc, signatureProvider, textDecoder: new TextDecoder(), textEncoder: new TextEncoder() });

const nftContract = 'your eos account';

async function createCol({ author, royalty, name, image, banner, description, links }) {
  if (!author || !royalty || !name || !image || !banner || typeof links != 'object') {
    throw new Error('Missing parameters');
  }
  const linksKV = [];
  if (typeof links == 'object') {
    for (const k in links) {
      linksKV.push({
        key: k,
        value: links[k]
      });
    }
  }
  const result = await api.transact({
    actions: [{
      account: nftContract,
      name: 'createcol',
      authorization: [{
        actor: nftContract,
        permission: 'active',
      }],
      data: {
        author,
        royalty,
        name,
        image,
        banner,
        description,
        links: linksKV,
      },
    }]
  }, {
    blocksBehind: 3,
    expireSeconds: 30,
  });
  return result;
};


async function createAsset({ collection_id, supply, max_supply, name, image, animation_url, external_url, description, attributes }) {
  if (!collection_id || !supply || !max_supply || !name || !image || !description || !Array.isArray(attributes) ) {
    throw new Error('Missing parameters');
  }
  // collection_id, supply, max_supply, name, image,  animation_url,  external_url, description, map<string, string> attributes
  if (!animation_url) {
    animation_url = '';
  }
  if (!external_url) {
    external_url = '';
  }
  const attributeKv = [];
  for (const attribute of attributes) {
    if (!attribute.trait_type) {
      if (attribute.key) {
        attribute.trait_type = attribute.key;
      } else if (attribute.type) {
        attribute.trait_type = attribute.type;
      } else {
        continue;
      }
    }
    attributeKv.push({
      key: attribute.trait_type,
      value: attribute.value,
    });
  }
  const result = await api.transact({
    actions: [{
      account: nftContract,
      name: 'createasset',
      authorization: [{
        actor: nftContract,
        permission: 'active',
      }],
      data: {
        collection_id,
        supply,
        max_supply,
        name,
        image,
        animation_url,
        external_url,
        description,
        attributes: attributeKv
      },
    }]
  }, {
    blocksBehind: 3,
    expireSeconds: 30,
  });
  return result;
};

async function mint(to, asset_id, amount, memo) {
  if (!asset_id || !amount || !to) {
    throw new Error('Missing parameters');
  }
  const result = await api.transact({
    actions: [{
      account: nftContract,
      name: 'mint',
      authorization: [{
        actor: nftContract,
        permission: 'active',
      }],
      data: {
        to,
        asset_id,
        amount,
        memo
      },
    }]
  }, {
    blocksBehind: 3,
    expireSeconds: 30,
  });
  return result;
};

async function transfer(from, to, asset_id, amount, memo) {
  if (!from || !to  || !asset_id || !amount) {
    throw new Error('Missing parameters');
  }
  const result = await api.transact({
    actions: [{
      account: nftContract,
      name: 'transfer',
      authorization: [{
        actor: nftContract,
        permission: 'active',
      }],
      data: {
        from,
        to,
        asset_id,
        amount,
        memo
      },
    }]
  }, {
    blocksBehind: 3,
    expireSeconds: 30,
  });
  return result;
}

export { createCol, createAsset, mint, transfer }